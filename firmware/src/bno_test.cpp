/*
 * Feather nRF52840 Express — BNO055 I2C terminal test
 *
 * Prints orientation, acceleration, gyroscope, and magnetometer to Serial.
 *
 * Build:  pio run -e bno_test -t upload
 * Monitor: screen /dev/cu.usbmodem1101 115200
 *
 * ── BNO055 wiring (I2C) ──────────────────────────────────────────────────────
 *   BNO055 VIN -> 3V
 *   BNO055 GND -> GND
 *   BNO055 SDA -> SDA (pin 25 on Feather nRF52840)
 *   BNO055 SCL -> SCL (pin 26 on Feather nRF52840)
 */

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

namespace {
  Adafruit_BNO055 bno(55, 0x28, &Wire);
}

void setup() {
  pinMode(LED_RED, OUTPUT);
  Serial.begin(115200);

  // Wait for screen to connect
  while (!Serial) {
    digitalWrite(LED_RED, HIGH);
    delay(100);
    digitalWrite(LED_RED, LOW);
    delay(100);
  }

  Serial.println("──────────────────────────────────────");
  Serial.println("  BNO055 I2C Test");
  Serial.println("──────────────────────────────────────");
  Serial.println("Initialising BNO055...");

  if (!bno.begin()) {
    Serial.println("ERROR: BNO055 not detected. Check wiring and I2C address.");
    while (true) {
      digitalWrite(LED_RED, HIGH);
      delay(100);
      digitalWrite(LED_RED, LOW);
      delay(100);
    }
  }

  bno.setExtCrystalUse(true);
  Serial.println("BNO055 OK\n");
}

void loop() {
  // ── Orientation (Euler angles) ───────────────────────────────────────────
  sensors_event_t orientEvent;
  bno.getEvent(&orientEvent, Adafruit_BNO055::VECTOR_EULER);

  // ── Acceleration (m/s²) ──────────────────────────────────────────────────
  sensors_event_t accelEvent;
  bno.getEvent(&accelEvent, Adafruit_BNO055::VECTOR_ACCELEROMETER);

  // ── Gyroscope (rad/s) ────────────────────────────────────────────────────
  sensors_event_t gyroEvent;
  bno.getEvent(&gyroEvent, Adafruit_BNO055::VECTOR_GYROSCOPE);

  // ── Magnetometer (µT) ────────────────────────────────────────────────────
  sensors_event_t magEvent;
  bno.getEvent(&magEvent, Adafruit_BNO055::VECTOR_MAGNETOMETER);

  // ── Calibration status ───────────────────────────────────────────────────
  uint8_t sysCal, gyroCal, accelCal, magCal;
  bno.getCalibration(&sysCal, &gyroCal, &accelCal, &magCal);

  Serial.println("──────────────────────────────────────");

  Serial.print("Calibration  sys:");  Serial.print(sysCal);
  Serial.print(" gyro:");             Serial.print(gyroCal);
  Serial.print(" accel:");            Serial.print(accelCal);
  Serial.print(" mag:");              Serial.println(magCal);

  Serial.println();

  Serial.print("Orientation  X:");   Serial.print(orientEvent.orientation.x, 2);
  Serial.print("°  Y:");             Serial.print(orientEvent.orientation.y, 2);
  Serial.print("°  Z:");             Serial.print(orientEvent.orientation.z, 2);
  Serial.println("°");

  Serial.print("Accel        X:");   Serial.print(accelEvent.acceleration.x, 2);
  Serial.print("  Y:");              Serial.print(accelEvent.acceleration.y, 2);
  Serial.print("  Z:");              Serial.print(accelEvent.acceleration.z, 2);
  Serial.println(" m/s²");

  Serial.print("Gyro         X:");   Serial.print(gyroEvent.gyro.x, 2);
  Serial.print("  Y:");              Serial.print(gyroEvent.gyro.y, 2);
  Serial.print("  Z:");              Serial.print(gyroEvent.gyro.z, 2);
  Serial.println(" rad/s");

  Serial.print("Mag          X:");   Serial.print(magEvent.magnetic.x, 2);
  Serial.print("  Y:");              Serial.print(magEvent.magnetic.y, 2);
  Serial.print("  Z:");              Serial.print(magEvent.magnetic.z, 2);
  Serial.println(" µT");

  Serial.println();
  delay(500);
}