#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <SPI.h>

void setup() {
  Serial.begin(115200);
  delay(5000);  // just wait 5 seconds, no Serial check
  Serial.println("Hello from nRF52840");
}

void loop() {
  Serial.println("still alive");
  delay(1000);
}