/*
 * Feather nRF52840 Express — Rotary Encoder test
 *
 * Counts up/down with rotation, resets with button press.
 *
 * Build:  pio run -e encoder_test -t upload
 * Monitor: screen /dev/cu.usbmodem* 115200
 *
 * ── Rotary Encoder wiring ────────────────────────────────────────────────────
 *   3-pin side:
 *     Left pin   -> A0 (CLK)
 *     Middle pin -> GND
 *     Right pin  -> A1 (DT)
 *   2-pin side:
 *     Either pin -> GND
 *     Other pin  -> A2 (SW)
 */

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>

namespace {

constexpr uint8_t kClk = A3;
constexpr uint8_t kDt  = A0;
constexpr uint8_t kSw  = A4;

int  counter     = 0;
int  lastClk     = HIGH;
bool lastSwState = HIGH;

}  // namespace

void setup() {
  pinMode(LED_RED, OUTPUT);
  pinMode(kClk, INPUT_PULLUP);
  pinMode(kDt,  INPUT_PULLUP);
  pinMode(kSw,  INPUT_PULLUP);

  Serial.begin(115200);
  while (!Serial) {
    digitalWrite(LED_RED, HIGH);
    delay(100);
    digitalWrite(LED_RED, LOW);
    delay(100);
  }

  Serial.println("──────────────────────────────────────");
  Serial.println("  Rotary Encoder Test");
  Serial.println("  Rotate to count, press to reset");
  Serial.println("──────────────────────────────────────\n");
  Serial.print("Counter: "); Serial.println(counter);
}

void loop() {
  int clkState = digitalRead(kClk);
  bool swState = digitalRead(kSw);

  // ── Rotation detection ───────────────────────────────────────────────────
  if (clkState != lastClk && clkState == LOW) {
    if (digitalRead(kDt) != clkState) {
      counter++;
      Serial.print("Counter: "); Serial.print(counter);
      Serial.println("  (clockwise)");
    } else {
      counter--;
      Serial.print("Counter: "); Serial.print(counter);
      Serial.println("  (counter-clockwise)");
    }
    digitalWrite(LED_RED, HIGH);
    delay(10);
    digitalWrite(LED_RED, LOW);
  }
  lastClk = clkState;

  // ── Button press detection ───────────────────────────────────────────────
  if (swState == LOW && lastSwState == HIGH) {
    counter = 0;
    Serial.println("── Button pressed! Counter reset to 0 ──");
    // Blink 3 times to confirm reset
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_RED, HIGH);
      delay(100);
      digitalWrite(LED_RED, LOW);
      delay(100);
    }
  }
  lastSwState = swState;

  delay(5);
}