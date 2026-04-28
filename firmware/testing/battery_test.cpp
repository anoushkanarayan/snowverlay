#include <Arduino.h>
#include <U8g2lib.h>

// Feather nRF52840 VBAT is connected through a 2:1 divider.
// A6 reads half the battery voltage, so we multiply by 2.
constexpr uint8_t kVbatPin = A6;
constexpr uint8_t kOledCs  = 5;
constexpr uint8_t kOledDc  = 9;
constexpr uint8_t kOledRst = 6;

U8G2_SSD1309_128X64_NONAME2_F_4W_HW_SPI display(U8G2_R0, kOledCs, kOledDc, kOledRst);

float readBatteryVoltage() {
  float measuredVbat = analogRead(kVbatPin);
  measuredVbat *= 2.0f;     // Undo 2:1 divider on VBAT.
  measuredVbat *= 3.6f;     // ADC reference voltage used by Feather docs.
  measuredVbat /= 1024.0f;  // 10-bit ADC conversion scale.
  return measuredVbat;
}

int batteryPercentFromVoltage(float vbat) {
  constexpr float kEmptyV = 3.2f;
  constexpr float kFullV  = 4.2f;

  float pct = ((vbat - kEmptyV) / (kFullV - kEmptyV)) * 100.0f;
  if (pct < 0.0f) pct = 0.0f;
  if (pct > 100.0f) pct = 100.0f;
  return (int)(pct + 0.5f);  // Round to nearest integer.
}

void drawBatteryIndicator(int x, int y, int pct) {
  constexpr uint8_t kBodyW = 10;
  constexpr uint8_t kBodyH = 5;
  constexpr uint8_t kTipW  = 1;
  constexpr uint8_t kTipH  = 3;
  constexpr uint8_t kPad   = 1;

  // Battery outline + terminal.
  display.drawFrame(x, y, kBodyW, kBodyH);
  display.drawBox(x + kBodyW, y + (kBodyH - kTipH) / 2, kTipW, kTipH);

  // Filled level inside the battery body.
  int innerW = kBodyW - (kPad * 2);
  int fillW = (innerW * pct) / 100;
  if (fillW < 1 && pct > 0) fillW = 1;
  if (fillW > innerW) fillW = innerW;
  if (fillW > 0) {
    display.drawBox(x + kPad, y + kPad, fillW, kBodyH - (kPad * 2));
  }
}

void setup() {
  display.begin();
  display.setContrast(255);
}

void loop() {
  const float vbat = readBatteryVoltage();
  const int pct = batteryPercentFromVoltage(vbat);

  
  char pctLine[8];
  snprintf(pctLine, sizeof(pctLine), "%d%%", pct);

  display.clearBuffer();
  display.setFont(u8g2_font_4x6_tf);
  int startX = 2;  // left margin
  drawBatteryIndicator(startX, 1, pct);
  display.drawStr(startX + 13, 6, pctLine);
  display.sendBuffer();

  delay(1000);
}
