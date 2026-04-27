/*
 * snowverlay — main firmware
 *
 * Rotary encoder uses hardware interrupts for reliable rotation detection.
 * Button press toggles display on/off via interrupt.
 * All screens share GPS time/date in upper right.
 *
 * Screens:
 *   0: GPS    — Lat, Lon, Time, Date
 *   1: Motion — Speed, Acceleration, Compass
 *   2: Env    — Temp, Pressure, UV, Risk
 *
 * Build:  pio run -e main -t upload
 *
 * ── Pin assignments ───────────────────────────────────────────────────────────
 *   OLED CS  -> 5   OLED DC  -> 9   OLED RST -> 6
 *   GPS CS   -> 11  BME CS   -> 10
 *   BNO055   -> SDA/SCL (I2C)
 *   UV       -> A1
 *   Encoder CLK -> A3   DT -> A0   SW -> A4
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>
#include <U8g2lib.h>
#include <SparkFun_u-blox_GNSS_v3.h>
#include <Adafruit_BME680.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

namespace {

// ── Pins ─────────────────────────────────────────────────────────────────────
constexpr uint8_t kOledCs    = 5;
constexpr uint8_t kOledDc    = 9;
constexpr uint8_t kOledRst   = 6;
constexpr uint8_t kGpsCs     = 11;
constexpr uint8_t kBmeCs     = 10;
constexpr uint8_t kUvPin     = A1;
constexpr uint8_t kClk       = A3;
constexpr uint8_t kDt        = A0;
constexpr uint8_t kSw        = A4;
constexpr int8_t  kUtcOffset = -7;  // PDT

// ── Peripherals ───────────────────────────────────────────────────────────────
U8G2_SSD1309_128X64_NONAME2_F_4W_HW_SPI display(U8G2_R0, kOledCs, kOledDc, kOledRst);
SFE_UBLOX_GNSS_SPI gps;
Adafruit_BME680    bme(kBmeCs, &SPI);
Adafruit_BNO055    bno(55, 0x28, &Wire);

// ── State ─────────────────────────────────────────────────────────────────────
bool bmeOk = false, bnoOk = false, gpsOk = false;
constexpr uint8_t kNumScreens = 3;

// Volatile variables modified by interrupts
volatile int     currentScreen = 0;
volatile bool    displayOn     = true;
volatile bool    screenChanged = false;
volatile bool    displayToggled = false;

// Debounce timestamps
volatile uint32_t lastEncoderTime = 0;
volatile uint32_t lastSwTime      = 0;
constexpr uint32_t kEncoderDebounce = 5;   // ms
constexpr uint32_t kSwDebounce      = 200; // ms

// ── GPS data shared across all screens ────────────────────────────────────────
struct GpsData {
  double   lat = 0.0, lon = 0.0;
  float    speedMph = 0.0f;
  bool     fixValid = false, timeValid = false;
  uint8_t  hour = 0, minute = 0, month = 0, day = 0;
  uint16_t year = 0;
};
GpsData gData;

// ── Interrupt handlers ────────────────────────────────────────────────────────
void encoderISR() {
  uint32_t now = millis();
  if (now - lastEncoderTime < kEncoderDebounce) return;
  lastEncoderTime = now;

  if (digitalRead(kDt) != digitalRead(kClk)) {
    currentScreen = (currentScreen + 1) % kNumScreens;
  } else {
    currentScreen = (currentScreen - 1 + kNumScreens) % kNumScreens;
  }
  screenChanged = true;
}

void switchISR() {
  uint32_t now = millis();
  if (now - lastSwTime < 500) return;  // increase from 200ms to 500ms
  lastSwTime = now;
  
  // Verify pin is actually LOW (not just noise)
  if (digitalRead(kSw) != LOW) return;
  
  displayOn      = !displayOn;
  displayToggled = true;
}

// ── Helpers ───────────────────────────────────────────────────────────────────
float toUvIndex(int raw) {
  return (raw * (3.3f / 1023.0f)) / 0.1f;
}

const char* uvRisk(float index) {
  if      (index < 3)  return "Low";
  else if (index < 6)  return "Moderate";
  else if (index < 8)  return "High";
  else if (index < 11) return "Very High";
  else                 return "Extreme";
}

// ── Shared header: time & date upper right ────────────────────────────────────
void drawHeader() {
  display.setFont(u8g2_font_5x7_tf);
  char topBuf[24] = "--:---- --/--/--";
  if (gData.timeValid) {
    int h     = (int)gData.hour + kUtcOffset;
    uint8_t d = gData.day;
    uint8_t m = gData.month;
    if (h < 0) { h += 24; d--; if (d == 0) { m--; d = 31; } }
    const char* ampm = (h >= 12) ? "PM" : "AM";
    if (h == 0)      h = 12;
    else if (h > 12) h -= 12;
    snprintf(topBuf, sizeof(topBuf), "%d:%02u%s %u/%02u/%02u",
             h, gData.minute, ampm, m, d, gData.year % 100);
  }
  int topX = 128 - (strlen(topBuf) * 5) - 8;
  display.drawStr(topX, 8, topBuf);
  display.drawHLine(0, 10, 128);
}

// ── Screen 0: GPS ─────────────────────────────────────────────────────────────
void drawGps() {
  drawHeader();
  display.setFont(u8g2_font_5x7_tf);

  double absLat = (gData.lat < 0) ? -gData.lat : gData.lat;
  double absLon = (gData.lon < 0) ? -gData.lon : gData.lon;
  char latDir = (gData.lat < 0) ? 'S' : 'N';
  char lonDir = (gData.lon < 0) ? 'W' : 'E';
  uint16_t latDeg  = (uint16_t)absLat;
  uint32_t latFrac = (uint32_t)((absLat - latDeg) * 100000UL);
  uint16_t lonDeg  = (uint16_t)absLon;
  uint32_t lonFrac = (uint32_t)((absLon - lonDeg) * 100000UL);

  char latLine[22], lonLine[22];
  snprintf(latLine, sizeof(latLine), "Lat: %03u.%05lu %c",
           latDeg, (unsigned long)latFrac, latDir);
  snprintf(lonLine, sizeof(lonLine), "Lon: %03u.%05lu %c",
           lonDeg, (unsigned long)lonFrac, lonDir);

  display.drawStr(0, 24, latLine);
  display.drawStr(0, 34, lonLine);

  if (!gData.fixValid) {
    display.drawStr(32, 48, "Locating...");
  }
}

// ── Screen 1: Motion ─────────────────────────────────────────────────────────
void drawCompass(int cx, int cy, int r, float heading) {
  display.drawLine(cx, cy - r, cx, cy + r);
  display.drawLine(cx - r, cy, cx + r, cy);
  display.setFont(u8g2_font_5x7_tf);
  display.drawStr(cx - 2, cy - r - 1, "N");
  display.drawStr(cx - 2, cy + r + 7, "S");
  display.drawStr(cx + r + 2, cy + 3,  "E");
  display.drawStr(cx - r - 6, cy + 3,  "W");
  float rad = (heading - 90.0f) * M_PI / 180.0f;
  int nx = cx + (int)(cos(rad) * (r - 2));
  int ny = cy + (int)(sin(rad) * (r - 2));
  display.drawLine(cx, cy, nx, ny);
  display.drawBox(cx - 1, cy - 1, 3, 3);
}

void drawMotion(float accelXY, float heading) {
  drawHeader();
  display.setFont(u8g2_font_5x7_tf);

  char spdBuf[22], accelBuf[22];
  if (gpsOk && gData.fixValid)
    snprintf(spdBuf, sizeof(spdBuf), "Spd: %.1f mph", gData.speedMph);
  else
    snprintf(spdBuf, sizeof(spdBuf), "Spd: Locating...");

  if (bnoOk)
    snprintf(accelBuf, sizeof(accelBuf), "Accel: %.2f m/s2", accelXY);
  else
    snprintf(accelBuf, sizeof(accelBuf), "Accel: --");

  display.drawStr(0, 22, spdBuf);
  display.drawStr(0, 32, accelBuf);

  char hdgBuf[16] = "--";
  if (bnoOk) {
    const char* cardDir;
    if      (heading <  22.5f || heading >= 337.5f) cardDir = "N";
    else if (heading <  67.5f)                      cardDir = "NE";
    else if (heading < 112.5f)                      cardDir = "E";
    else if (heading < 157.5f)                      cardDir = "SE";
    else if (heading < 202.5f)                      cardDir = "S";
    else if (heading < 247.5f)                      cardDir = "SW";
    else if (heading < 292.5f)                      cardDir = "W";
    else                                            cardDir = "NW";
    snprintf(hdgBuf, sizeof(hdgBuf), "%.0f\xb0 %s", heading, cardDir);
  }
  display.drawStr(0, 44, "Heading:");
  display.drawStr(0, 54, hdgBuf);
  drawCompass(100, 40, 10, heading);
}

// ── Screen 2: Environment ─────────────────────────────────────────────────────
void drawEnv(float uvIndex, float tempC, float pressHpa) {
  drawHeader();
  display.setFont(u8g2_font_5x7_tf);

  char tempBuf[22], presBuf[22], uvBuf[22];
  if (bmeOk) {
    snprintf(tempBuf, sizeof(tempBuf), "Temp: %.1fF / %.1fC",
             tempC * 9.0f / 5.0f + 32.0f, tempC);
    snprintf(presBuf, sizeof(presBuf), "Pres: %.1f hPa", pressHpa);
  } else {
    snprintf(tempBuf, sizeof(tempBuf), "Temp: --");
    snprintf(presBuf, sizeof(presBuf), "Pres: --");
  }
  snprintf(uvBuf, sizeof(uvBuf), "UV: %.1f, Risk: %s",
           uvIndex, uvRisk(uvIndex));

  display.drawStr(0, 22, tempBuf);
  display.drawStr(0, 32, presBuf);
  display.drawStr(0, 42, uvBuf);
}

// ── Boot animation ────────────────────────────────────────────────────────────
struct Flake { float x, y, vy, vx; uint8_t size; };

void playBootAnimation() {
  constexpr uint8_t kMaxFlakes = 30;
  constexpr uint8_t kFrames    = 90;
  Flake flakes[kMaxFlakes];
  uint8_t flakeCount = 0;

  auto makeFlake = [](Flake& f) {
    f.x    = random(0, 128);
    f.y    = -2;
    f.vy   = 0.4f + random(0, 10) * 0.04f;
    f.vx   = (random(0, 10) - 5) * 0.06f;
    f.size = (random(0, 10) < 3) ? 2 : 1;
  };

  for (uint8_t i = 0; i < 8; i++) {
    makeFlake(flakes[i]);
    flakes[i].y = random(0, 64);
  }
  flakeCount = 8;

  for (uint8_t frame = 0; frame < kFrames; frame++) {
    display.clearBuffer();
    for (uint8_t i = 0; i < flakeCount; i++) {
      flakes[i].x += flakes[i].vx;
      flakes[i].y += flakes[i].vy;
      if (flakes[i].x < 0)   flakes[i].x = 127;
      if (flakes[i].x > 127) flakes[i].x = 0;
      if (flakes[i].y > 66)  makeFlake(flakes[i]);
      display.drawBox((int)flakes[i].x, (int)flakes[i].y,
                      flakes[i].size, flakes[i].size);
    }
    if (frame % 8 == 0 && flakeCount < kMaxFlakes)
      makeFlake(flakes[flakeCount++]);
    if (frame > 30) {
      display.setFont(u8g2_font_ncenB10_tf);
      display.drawStr(14, 30, "snowverlay");
      display.setFont(u8g2_font_5x7_tf);
      display.drawStr(24, 45, "initializing...");
    }
    display.sendBuffer();
    delay(33);
  }
}

}  // namespace

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
  pinMode(kGpsCs,  OUTPUT); digitalWrite(kGpsCs,  HIGH);
  pinMode(kBmeCs,  OUTPUT); digitalWrite(kBmeCs,  HIGH);
  pinMode(kOledCs, OUTPUT); digitalWrite(kOledCs, HIGH);
  pinMode(kUvPin,  INPUT);
  pinMode(kClk,    INPUT_PULLUP);
  pinMode(kDt,     INPUT_PULLUP);
  pinMode(kSw,     INPUT_PULLUP);
  pinMode(LED_RED, OUTPUT);

  // Attach interrupts
  attachInterrupt(digitalPinToInterrupt(kClk), encoderISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(kSw),  switchISR,  FALLING);

  display.begin();
  display.setContrast(255);
  playBootAnimation();

  Wire.begin();

  gpsOk = gps.begin(SPI, kGpsCs, 5000000);
  if (gpsOk) gps.setVal8(UBLOX_CFG_MSGOUT_UBX_NAV_PVT_SPI, 1);

  bmeOk = bme.begin();
  if (bmeOk) {
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150);
  }

  bnoOk = bno.begin();
  if (bnoOk) bno.setExtCrystalUse(true);
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
  // ── Handle display toggle from ISR ──────────────────────────────────
  if (displayToggled) {
    displayToggled = false;
    if (displayOn) {
      display.setPowerSave(0);
      playBootAnimation();
    } else {
      display.setPowerSave(1);
    }
  }

  // ── Poll GPS ─────────────────────────────────────────────────────────
  if (gpsOk && gps.getPVT()) {
    uint8_t fixType = gps.getFixType();
    gData.fixValid  = (fixType >= 2);
    gData.lat       = gps.getLatitude()  / 1e7;
    gData.lon       = gps.getLongitude() / 1e7;
    gData.speedMph  = gps.getGroundSpeed() / 1000.0f * 2.23694f;
    gData.timeValid = gps.getTimeValid() && gps.getDateValid();
    gData.hour      = gps.getHour();
    gData.minute    = gps.getMinute();
    gData.year      = gps.getYear();
    gData.month     = gps.getMonth();
    gData.day       = gps.getDay();
  }

  // ── Poll BNO055 ──────────────────────────────────────────────────────
  float accelXY = 0, heading = 0;
  if (bnoOk) {
    sensors_event_t accelEvent, orientEvent;
    bno.getEvent(&accelEvent, Adafruit_BNO055::VECTOR_ACCELEROMETER);
    bno.getEvent(&orientEvent, Adafruit_BNO055::VECTOR_EULER);
    float ax = accelEvent.acceleration.x;
    float ay = accelEvent.acceleration.y;
    accelXY = sqrt(ax*ax + ay*ay);
    heading = orientEvent.orientation.x;
  }

  // ── Poll BME688 + UV ─────────────────────────────────────────────────
  float tempC = 0, pressHpa = 0, uvIndex = 0;
  if (bmeOk && bme.performReading()) {
    tempC    = bme.temperature;
    pressHpa = bme.pressure / 100.0f;
  }
  int sum = 0;
  for (int i = 0; i < 10; i++) { sum += analogRead(kUvPin); delay(2); }
  uvIndex = toUvIndex(sum / 10);

  // ── Render ───────────────────────────────────────────────────────────
  if (displayOn) {
    display.clearBuffer();
    switch (currentScreen) {
      case 0: drawGps();                         break;
      case 1: drawMotion(accelXY, heading);      break;
      case 2: drawEnv(uvIndex, tempC, pressHpa); break;
    }
    display.sendBuffer();
  }
}