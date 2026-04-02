/*
 * Feather nRF52840 Express + 1.51" transparent OLED (typical: Waveshare SSD1309, 128x64)
 *
 * Build: pio run -e display
 * Upload: pio run -e display -t upload
 *
 * Wiring (SPI — default on many 1.51" boards):
 *   OLED VCC  -> Feather 3V
 *   OLED GND  -> Feather GND
 *   OLED DIN  -> MOSI (hardware SPI)
 *   OLED CLK  -> SCK  (hardware SPI)
 *   OLED CS   -> pin set in kOledCs (see below)
 *   OLED DC   -> kOledDc
 *   OLED RST  -> kOledRst (or use U8X8_PIN_NONE if RST is tied high on the module)
 *
 * I2C: move module jumpers to I2C, then swap to U8G2_SSD1309_128X64_NONAME2_F_HW_I2C + Wire.
 */

#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>

namespace {

constexpr uint8_t kOledCs = 9;
constexpr uint8_t kOledDc = 11;
constexpr uint8_t kOledRst = 10;

U8G2_SSD1309_128X64_NONAME2_F_4W_HW_SPI display(U8G2_R0, kOledCs, kOledDc, kOledRst);

}  // namespace

void setup() {
  display.begin();
  display.setContrast(255);
}

void loop() {
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tf);
  display.drawStr(0, 12, "Hello from");
  display.drawStr(0, 28, "snowverlay");
  display.sendBuffer();
  delay(500);
}
