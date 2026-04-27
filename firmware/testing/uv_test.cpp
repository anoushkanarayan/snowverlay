/*
 * Feather nRF52840 Express
 *   + 1.51" transparent OLED (SSD1309, 128x64, SPI)
 *   + UV Sensor (analog, pin A1)
 *
 * Build:  pio run -e uv_display -t upload
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

void renderDisplay(float uvIndex) {
  display.setFont(u8g2_font_5x7_tf);

  // ── Time placeholder (right-aligned) — swap with GPS time when integrated
  // For now shows static placeholder until GPS is connected
  char topBuf[24] = "--:---- --/--/--";
  int topX = 128 - (strlen(topBuf) * 5) - 8;
  display.drawStr(topX, 8, topBuf);

  display.drawHLine(0, 10, 128);

  // ── UV Index ─────────────────────────────────────────────────────────
  char uvBuf[16];
  snprintf(uvBuf, sizeof(uvBuf), "UV: %.1f", uvIndex);
  display.setFont(u8g2_font_5x7_tf);
  display.drawStr(0, 24, uvBuf);

  // ── Risk level ───────────────────────────────────────────────────────
  display.setFont(u8g2_font_5x7_tf);
  display.drawStr(0, 34, uvRisk(uvIndex));

  // ── Bar graph ────────────────────────────────────────────────────────
  int barWidth = (int)((uvIndex / 11.0f) * 118.0f);
  if (barWidth > 118) barWidth = 118;
  display.setFont(u8g2_font_5x7_tf);
  display.drawFrame(5, 40, 118, 6);
  if (barWidth > 0) display.drawBox(5, 40, barWidth, 6);
}

}  // namespace

void setup() {
  pinMode(kUvPin, INPUT);
  pinMode(kOledCs, OUTPUT); digitalWrite(kOledCs, HIGH);

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
    delay(5);
  }
  float uvIndex = toUvIndex(sum / 10);

  display.clearBuffer();
  renderDisplay(uvIndex);
  display.sendBuffer();

  delay(1000);
}