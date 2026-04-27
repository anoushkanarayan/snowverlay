/*
 * Feather nRF52840 Express
 *   + 1.51" transparent OLED (SSD1309, 128x64, SPI)
 *   + SparkFun NEO-M9N GPS (SPI)
 *   + BNO055 (I2C)
 *
 * Displays: GPS speed, XY acceleration magnitude, compass cross
 *
 * Build:  pio run -e motion_display -t upload
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>
#include <U8g2lib.h>
#include <SparkFun_u-blox_GNSS_v3.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

namespace {

constexpr uint8_t kOledCs   = 5;
constexpr uint8_t kOledDc   = 9;
constexpr uint8_t kOledRst  = 6;
constexpr uint8_t kGpsCs    = 11;
constexpr int8_t  kUtcOffset = -7;  // PDT

U8G2_SSD1309_128X64_NONAME2_F_4W_HW_SPI display(U8G2_R0, kOledCs, kOledDc, kOledRst);
SFE_UBLOX_GNSS_SPI gps;
Adafruit_BNO055    bno(55, 0x28, &Wire);

bool bnoOk = false;
bool gpsOk = false;

struct GpsData {
  float    speedMph  = 0.0f;
  bool     fixValid  = false;
  bool     timeValid = false;
  uint8_t  hour = 0, minute = 0, month = 0, day = 0;
  uint16_t year = 0;
};

GpsData gData;

// Draw a compass cross centered at (cx, cy) with radius r
// heading: 0=N, 90=E, 180=S, 270=W
void drawCompass(int cx, int cy, int r, float heading) {
  // Draw cross arms
  display.drawLine(cx, cy - r, cx, cy + r);  // vertical
  display.drawLine(cx - r, cy, cx + r, cy);  // horizontal

  // Cardinal labels
  display.setFont(u8g2_font_5x8_tr);
  display.drawStr(cx - 2, cy - r - 1, "N");
  display.drawStr(cx - 2, cy + r + 7, "S");
  display.drawStr(cx + r + 2, cy + 3,  "E");
  display.drawStr(cx - r - 6, cy + 3,  "W");

  // Direction needle — arrow pointing toward current heading
  float rad = (heading - 90.0f) * M_PI / 180.0f;
  int nx = cx + (int)(cos(rad) * (r - 2));
  int ny = cy + (int)(sin(rad) * (r - 2));
  display.drawLine(cx, cy, nx, ny);

  // Center dot
  display.drawBox(cx - 1, cy - 1, 3, 3);
}

void renderDisplay(float accelXY, float heading) {
  display.setFont(u8g2_font_5x8_tr);

  // ── Time & Date upper right ──────────────────────────────────────────
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

  // ── GPS Speed ────────────────────────────────────────────────────────
  char spdBuf[22];
  if (gpsOk && gData.fixValid) {
    snprintf(spdBuf, sizeof(spdBuf), "Spd: %.1f mph", gData.speedMph);
  } else {
    snprintf(spdBuf, sizeof(spdBuf), "Spd: Locating...");
  }
  display.drawStr(0, 20, spdBuf);

  // ── XY Acceleration magnitude ─────────────────────────────────────
  char accelBuf[22];
  if (bnoOk) {
    snprintf(accelBuf, sizeof(accelBuf), "Accel: %.2f m/s2", accelXY);
  } else {
    snprintf(accelBuf, sizeof(accelBuf), "Accel: --");
  }
  display.drawStr(0, 30, accelBuf);

  // ── Compass cross (right side) ───────────────────────────────────────
  // Center at x=100, y=47, radius=14
  drawCompass(100, 33, 10, heading);

  // ── Heading value (left side) ────────────────────────────────────────
  char hdgBuf[16];
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
  } else {
    snprintf(hdgBuf, sizeof(hdgBuf), "--");
  }
  display.drawStr(0, 42, "Heading:");
  display.drawStr(0, 52, hdgBuf);
}

}  // namespace

void setup() {
  pinMode(kGpsCs,  OUTPUT); digitalWrite(kGpsCs,  HIGH);
  pinMode(kOledCs, OUTPUT); digitalWrite(kOledCs, HIGH);

  display.begin();
  display.setContrast(255);
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tf);
  display.drawStr(0, 12, "snowverlay");
  display.drawStr(0, 28, "Motion init...");
  display.sendBuffer();

  Wire.begin();

  gpsOk = gps.begin(SPI, kGpsCs, 5000000);
  if (gpsOk) gps.setVal8(UBLOX_CFG_MSGOUT_UBX_NAV_PVT_SPI, 1);

  bnoOk = bno.begin();
  if (bnoOk) bno.setExtCrystalUse(true);
}

void loop() {
  // ── Poll GPS ─────────────────────────────────────────────────────────
  if (gpsOk && gps.getPVT()) {
    uint8_t fixType = gps.getFixType();
    gData.fixValid  = (fixType >= 2);
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
    // XY acceleration magnitude √(x²+y²)
    sensors_event_t accelEvent;
    bno.getEvent(&accelEvent, Adafruit_BNO055::VECTOR_ACCELEROMETER);
    float ax = accelEvent.acceleration.x;
    float ay = accelEvent.acceleration.y;
    accelXY = sqrt(ax*ax + ay*ay);

    // Heading from Euler X (0=N, 90=E, 180=S, 270=W)
    sensors_event_t orientEvent;
    bno.getEvent(&orientEvent, Adafruit_BNO055::VECTOR_EULER);
    heading = orientEvent.orientation.x;
  }

  display.clearBuffer();
  renderDisplay(accelXY, heading);
  display.sendBuffer();

  delay(100);
}