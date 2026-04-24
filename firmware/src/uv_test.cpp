/*
 * Feather nRF52840 Express — UV Sensor terminal test
 * EC Buying 2PCS 3.3V-5V UV Sensor Module 200-370nm
 *
 * Build:  pio run -e uv_test -t upload
 * Monitor: screen /dev/cu.usbmodem1101 115200
 *
 * ── UV Sensor wiring ─────────────────────────────────────────────────────────
 *   UV VCC  -> 3V
 *   UV GND  -> GND
 *   UV OUT  -> A1
 */

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>

namespace {

constexpr uint8_t kUvPin = A1;

// Convert raw ADC reading to UV index
// Based on GUVA-S12SD response: ~0.1V per UV index unit at 3.3V reference
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

}  // namespace

void setup() {
  pinMode(LED_RED, OUTPUT);
  pinMode(kUvPin, INPUT);
  Serial.begin(115200);

  while (!Serial) {
    digitalWrite(LED_RED, HIGH);
    delay(100);
    digitalWrite(LED_RED, LOW);
    delay(100);
  }

  Serial.println("──────────────────────────────────────");
  Serial.println("  UV Sensor Test");
  Serial.println("──────────────────────────────────────\n");
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

  Serial.println("──────────────────────────────────────");
  Serial.print("Raw value   : "); Serial.println(raw);
  Serial.print("Voltage     : "); Serial.print(voltage, 3); Serial.println(" V");
  Serial.print("UV Index    : "); Serial.println(uvIndex, 1);
  Serial.print("Risk level  : "); Serial.println(uvRisk(uvIndex));
  Serial.println();

  delay(1000);
}