/*
 * Feather nRF52840 Express — BME688 SPI terminal test
 *
 * Prints temperature, humidity, pressure, and gas resistance to Serial.
 *
 * Build:  pio run -e bme_test -t upload
 * Monitor: screen /dev/cu.usbmodem* 115200
 *
 * ── BME688 wiring (SPI) ───────────────────────────────────────────────────────
 *   BME688 VCC  -> 3V
 *   BME688 GND  -> GND
 *   BME688 SDI  -> MOSI
 *   BME688 SCK  -> SCK
 *   BME688 SDO  -> MISO
 *   BME688 CS   -> pin 10
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_BME680.h>

namespace {
  constexpr uint8_t kBmeCs = 10;
  Adafruit_BME680 bme(kBmeCs, &SPI);
}

void setup() {
  pinMode(LED_RED, OUTPUT);
  Serial.begin(115200);

  // Wait for screen to connect — LED rapid blinks while waiting
  while (!Serial) {
    digitalWrite(LED_RED, HIGH);
    delay(100);
    digitalWrite(LED_RED, LOW);
    delay(100);
  }

  Serial.println("──────────────────────────────────────");
  Serial.println("  BME688 SPI Test");
  Serial.println("──────────────────────────────────────");
  Serial.println("Initialising BME688...");

  if (!bme.begin()) {
    Serial.println("ERROR: BME688 not detected. Check wiring and CS pin.");
    while (true) delay(1000);
  }

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);

  Serial.println("BME688 OK\n");
}

void loop() {
  if (!bme.performReading()) {
    Serial.println("ERROR: Failed to perform reading.");
    delay(1000);
    return;
  }

  Serial.println("──────────────────────────────────────");

  Serial.print("Temperature : ");
  Serial.print(bme.temperature, 2);
  Serial.print(" C  /  ");
  Serial.print(bme.temperature * 9.0 / 5.0 + 32.0, 2);
  Serial.println(" F");

  Serial.print("Humidity    : ");
  Serial.print(bme.humidity, 2);
  Serial.println(" %");

  Serial.print("Pressure    : ");
  Serial.print(bme.pressure / 100.0, 2);
  Serial.println(" hPa");

  Serial.print("Gas resist  : ");
  Serial.print(bme.gas_resistance / 1000.0, 2);
  Serial.println(" kOhm");

  Serial.println();
  delay(2000);
}