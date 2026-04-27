/*
 * Feather nRF52840 Express — onboard LEDs only (no display).
 *
 * Cycles the RGB NeoPixel through colors and toggles the red / blue status LEDs.
 *
 * Build:  pio run -e led_demo
 * Upload: pio run -e led_demo -t upload
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

namespace {

constexpr uint8_t kNeoPixelCount = 1;
constexpr uint8_t kNeoBrightness = 40;

Adafruit_NeoPixel sNeo(kNeoPixelCount, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

void neoRgb(uint8_t r, uint8_t g, uint8_t b) {
  sNeo.setPixelColor(0, Adafruit_NeoPixel::Color(r, g, b));
  sNeo.show();
}

}  // namespace

void setup() {
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  sNeo.begin();
  sNeo.setBrightness(kNeoBrightness);
  sNeo.clear();
  sNeo.show();
}

void loop() {
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_BLUE, LOW);
  neoRgb(255, 0, 0);
  delay(350);

  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, HIGH);
  neoRgb(0, 255, 0);
  delay(350);

  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, LOW);
  neoRgb(0, 100, 255);
  delay(350);

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_BLUE, HIGH);
  neoRgb(255, 0, 255);
  delay(350);

  neoRgb(0, 0, 0);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, LOW);
  delay(200);
}
