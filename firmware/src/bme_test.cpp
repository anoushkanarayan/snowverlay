/*
 * Feather nRF52840 Express — BME688 SPI terminal test
 *
 * Prints temperature, humidity, pressure, and gas resistance to Serial.
 *
 * Build:  pio run -e bme_test
 * Upload: pio run -e bme_test -t upload
 *
 * platformio.ini env:
 *   [env:bme_test]
 *   platform = nordicnrf52
 *   board = adafruit_feather_nrf52840
 *   framework = arduino
 *   monitor_speed = 115200
 *   build_flags = ${common.build_flags}
 *   lib_deps =
 *       adafruit/Adafruit BME680 Library
 *       adafruit/Adafruit TinyUSB Library
 *   build_src_filter = +<bme_test.cpp>
 *
 * ── BME688 wiring (SPI) ───────────────────────────────────────────────────────
 *   BME688 VCC  -> 3V
 *   BME688 GND  -> GND
 *   BME688 SDI  -> MOSI
 *   BME688 SCK  -> SCK
 *   BME688 SDO  -> MISO
 *   BME688 CS   -> kBmeCs (pin 11) — change if wired differently
 */

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <SPI.h>
#include <Adafruit_BME680.h>

namespace {
  constexpr uint8_t kBmeCs = 10;   // Change if wired differently
  Adafruit_BME680 bme(kBmeCs, &SPI);
}

void setup() {
  Serial.begin(115200);

  // Wait up to 3 seconds for Serial Monitor, then continue anyway
  uint32_t t = millis();
  while (!Serial && millis() - t < 3000) delay(10);

  Serial.println("──────────────────────────────────────");
  Serial.println("  BME688 SPI Test");
  Serial.println("──────────────────────────────────────");
  Serial.println("Initialising BME688...");

  if (!bme.begin(kBmeCs)) {
    Serial.println("ERROR: BME688 not detected. Check wiring and CS pin.");
    while (true) delay(1000);
  }

  // Oversampling and filter settings
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);

  // Gas heater — 320°C for 150ms
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
  Serial.print(" °C  /  ");
  Serial.print(bme.temperature * 9.0 / 5.0 + 32.0, 2);
  Serial.println(" °F");

  Serial.print("Humidity    : ");
  Serial.print(bme.humidity, 2);
  Serial.println(" %");

  Serial.print("Pressure    : ");
  Serial.print(bme.pressure / 100.0, 2);
  Serial.println(" hPa");

  Serial.print("Gas resist  : ");
  Serial.print(bme.gas_resistance / 1000.0, 2);
  Serial.println(" kΩ");

  Serial.println();

  delay(8000);
}