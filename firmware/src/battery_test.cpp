/*
 * Feather nRF52840 Express — Battery Voltage Monitor
 *
 * Reads battery voltage and estimates remaining percentage.
 * Logs readings with timestamp so you can track battery life over time.
 *
 * Build:  pio run -e battery_test -t upload
 * Monitor: screen /dev/cu.usbmodem* 115200
 */

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>

namespace {

constexpr uint8_t kVBatPin = A6;

// LiPo voltage ranges (approximate)
// 4.2V = 100%, 3.7V = 50%, 3.2V = 0%
float batteryPercent(float voltage) {
  if (voltage >= 4.2f) return 100.0f;
  if (voltage >= 4.0f) return map(voltage * 100, 400, 420, 75, 100);
  if (voltage >= 3.7f) return map(voltage * 100, 370, 400, 50, 75);
  if (voltage >= 3.5f) return map(voltage * 100, 350, 370, 25, 50);
  if (voltage >= 3.2f) return map(voltage * 100, 320, 350, 0,  25);
  return 0.0f;
}

const char* batteryStatus(float voltage) {
  if (voltage >= 4.0f) return "Full";
  if (voltage >= 3.7f) return "Good";
  if (voltage >= 3.5f) return "Low";
  return "Critical";
}

uint32_t startTime = 0;
uint32_t readingCount = 0;

}  // namespace

void setup() {
  pinMode(LED_RED, OUTPUT);
  pinMode(kVBatPin, INPUT);
  Serial.begin(115200);

  while (!Serial) {
    digitalWrite(LED_RED, HIGH);
    delay(100);
    digitalWrite(LED_RED, LOW);
    delay(100);
  }

  startTime = millis();

  Serial.println("──────────────────────────────────────────");
  Serial.println("  Battery Life Monitor");
  Serial.println("  Feather nRF52840 Express");
  Serial.println("──────────────────────────────────────────");
  Serial.println("Time(s)  Voltage  Percent  Status");
  Serial.println("──────────────────────────────────────────");
}

void loop() {
  // Read battery voltage using Adafruit's method
  float measuredVbat = analogRead(kVBatPin);
  measuredVbat *= 2;     // divided by 2 on board, multiply back
  measuredVbat *= 3.6f;  // reference voltage
  measuredVbat /= 1024;  // convert to voltage

  float pct    = batteryPercent(measuredVbat);
  const char* status = batteryStatus(measuredVbat);
  uint32_t elapsedSec = (millis() - startTime) / 1000;

  // Print CSV-friendly format for easy graphing later
  char line[60];
  snprintf(line, sizeof(line), "%-8lu %-8.3f %-8.1f %s",
           elapsedSec, measuredVbat, pct, status);
  Serial.println(line);

  readingCount++;

  // Blink LED based on battery level
  // Fast = low battery, slow = good
  int blinkDelay = (int)(pct * 5) + 100;
  digitalWrite(LED_RED, HIGH);
  delay(blinkDelay);
  digitalWrite(LED_RED, LOW);

  // Read every 60 seconds for long-term logging
  delay(60000);
}