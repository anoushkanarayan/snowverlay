/*
 * Feather nRF52840 Express
 *   + 1.51" transparent OLED (SSD1309, 128x64, SPI)
 *   + UV Sensor (analog)
 *
 * Build:  pio run -e uv_display -t upload
 *
 * ── OLED wiring (SPI) ────────────────────────────────────────────────────────
 *   OLED CS  -> pin 5
 *   OLED DC  -> pin 9
 *   OLED RST -> pin 6
 *
 * ── UV Sensor wiring ─────────────────────────────────────────────────────────
 *   UV VCC  -> 3V
 *   UV GND  -> GND
 *   UV OUT  -> A1
 */

#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>

namespace {

constexpr uint8_t kOledCs  = 5;
constexpr uint8_t kOledDc  = 9;
constexpr uint8_t kOledRst = 6;
constexpr uint8_t kUvPin   = A1;

U8G2_SSD1309_128X64_NONAME2_F_4W_HW_SPI display(U8G2_R0, kOledCs, kOledDc, kOledRst);

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

// Draw a simple bar graph for UV index (0-11+ scale)
void drawUvBar(float uvIndex) {
  int barWidth = (int)((uvIndex / 11.0f) * 118.0f);
  if (barWidth > 118) barWidth = 118;
  display.drawFrame(5, 44, 118, 10);
  if (barWidth > 0) display.drawBox(5, 44, barWidth, 10);
}

}  // namespace

void setup() {
  pinMode(kUvPin, INPUT);
  display.begin();
  display.setContrast(255);
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tf);
  display.drawStr(0, 12, "snowverlay");
  display.drawStr(0, 28, "UV Sensor init...");
  display.sendBuffer();
  delay(1000);
}

void loop() {
  // Average 10 readings to reduce noise
  int sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(kUvPin);
    delay(10);
  }
  int   raw     = sum / 10;
  float voltage = raw * (3.3f / 1023.0f);
  float uvIndex = toUvIndex(raw);

  char uvBuf[20], voltBuf[20], rawBuf[20];
  snprintf(uvBuf,   sizeof(uvBuf),   "UV Index: %.1f", uvIndex);
  snprintf(voltBuf, sizeof(voltBuf), "Voltage:  %.3f V", voltage);
  snprintf(rawBuf,  sizeof(rawBuf),  "Raw: %d", raw);

  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tf);
  display.drawStr(0, 10, "-- UV Sensor --");
  display.drawHLine(0, 12, 128);
  display.setFont(u8g2_font_5x7_tf);
  display.drawStr(0, 24, uvBuf);
  display.drawStr(0, 34, voltBuf);
  display.drawStr(0, 42, rawBuf);
  drawUvBar(uvIndex);
  display.drawStr(0, 64, uvRisk(uvIndex));
  display.sendBuffer();

  delay(1000);
}