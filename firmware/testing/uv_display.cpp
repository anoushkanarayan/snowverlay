/*
 * Feather nRF52840 Express
 *   + 1.51" transparent OLED (SSD1309, 128x64, SPI)
 *   + UV Sensor (analog, pin A1)
 *   + BME688 (SPI, pin 10)
 *
 * Build:  pio run -e uv_display -t upload
 */

#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Adafruit_BME680.h>

namespace {

constexpr uint8_t kOledCs  = 5;
constexpr uint8_t kOledDc  = 9;
constexpr uint8_t kOledRst = 6;
constexpr uint8_t kUvPin   = A1;
constexpr uint8_t kBmeCs   = 10;

U8G2_SSD1309_128X64_NONAME2_F_4W_HW_SPI display(U8G2_R0, kOledCs, kOledDc, kOledRst);
Adafruit_BME680 bme(kBmeCs, &SPI);

bool bmeOk = false;

float toUvIndex(int raw) {
  float voltage = raw * (3.3f / 1023.0f);
  return voltage / 0.1f;
}

const char* uvRisk(float index) {
  if      (index < 3)  return "Low";
  else if (index < 6)  return "Moderate";
  else if (index < 8)  return "High";
  else if (index < 11) return "Very High";
  else                 return "Extreme";
}

void renderDisplay(float uvIndex, float tempC, float pressHpa) {
  display.setFont(u8g2_font_5x7_tf);

  // ── Time placeholder upper right ─────────────────────────────────────
  char topBuf[24] = "--:---- --/--/--";
  int topX = 128 - (strlen(topBuf) * 5) - 8;
  display.drawStr(topX, 8, topBuf);
  display.drawHLine(0, 10, 128);

  // ── Temperature ──────────────────────────────────────────────────────
  char tempBuf[22];
  if (bmeOk) {
    snprintf(tempBuf, sizeof(tempBuf), "Temp: %.1fF / %.1fC",
             tempC * 9.0f / 5.0f + 32.0f, tempC);
  } else {
    snprintf(tempBuf, sizeof(tempBuf), "Temp: --");
  }
  display.drawStr(0, 22, tempBuf);

  // ── Pressure ─────────────────────────────────────────────────────────
  char presBuf[22];
  if (bmeOk) {
    snprintf(presBuf, sizeof(presBuf), "Pres: %.1f hPa", pressHpa);
  } else {
    snprintf(presBuf, sizeof(presBuf), "Pres: --");
  }
  display.drawStr(0, 32, presBuf);

  // ── UV & Risk on same line ────────────────────────────────────────────
  char uvBuf[22];
  snprintf(uvBuf, sizeof(uvBuf), "UV: %.1f, Risk: %s", uvIndex, uvRisk(uvIndex));
  display.drawStr(0, 42, uvBuf);
}

}  // namespace

void setup() {
  pinMode(kUvPin,  INPUT);
  pinMode(kBmeCs,  OUTPUT); digitalWrite(kBmeCs,  HIGH);
  pinMode(kOledCs, OUTPUT); digitalWrite(kOledCs, HIGH);

  display.begin();
  display.setContrast(255);
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tf);
  display.drawStr(0, 12, "snowverlay");
  display.drawStr(0, 28, "Sensor init...");
  display.sendBuffer();

  bmeOk = bme.begin();
  if (bmeOk) {
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150);
  }

  delay(500);
}

void loop() {
  int sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(kUvPin);
    delay(5);
  }
  float uvIndex = toUvIndex(sum / 10);

  float tempC = 0, pressHpa = 0;
  if (bmeOk && bme.performReading()) {
    tempC    = bme.temperature;
    pressHpa = bme.pressure / 100.0f;
  }

  display.clearBuffer();
  renderDisplay(uvIndex, tempC, pressHpa);
  display.sendBuffer();

  delay(1000);
}