/*
 * Feather nRF52840 Express
 *   + 1.51" transparent OLED (SSD1309, 128x64, SPI)
 *   + BNO055 (I2C)
 *
 * Displays orientation, accel, gyro, and mag on OLED.
 *
 * Build:  pio run -e bno_display -t upload
 *
 * ── OLED wiring (SPI) ────────────────────────────────────────────────────────
 *   OLED CS  -> pin 5
 *   OLED DC  -> pin 9
 *   OLED RST -> pin 6
 *
 * ── BNO055 wiring (I2C) ──────────────────────────────────────────────────────
 *   BNO055 VIN -> 3V
 *   BNO055 GND -> GND
 *   BNO055 SDA -> SDA
 *   BNO055 SCL -> SCL
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

namespace {

constexpr uint8_t kOledCs  = 5;
constexpr uint8_t kOledDc  = 9;
constexpr uint8_t kOledRst = 6;

U8G2_SSD1309_128X64_NONAME2_F_4W_HW_SPI display(U8G2_R0, kOledCs, kOledDc, kOledRst);
Adafruit_BNO055 bno(55, 0x28, &Wire);

}  // namespace

void setup() {
  display.begin();
  display.setContrast(255);
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tf);
  display.drawStr(0, 12, "snowverlay");
  display.drawStr(0, 28, "BNO055 init...");
  display.sendBuffer();

  if (!bno.begin()) {
    display.clearBuffer();
    display.drawStr(0, 12, "BNO055 error!");
    display.drawStr(0, 28, "Check wiring.");
    display.sendBuffer();
    while (true) delay(1000);
  }

  bno.setExtCrystalUse(true);
}

void loop() {
  sensors_event_t orientEvent, accelEvent, gyroEvent, magEvent;
  bno.getEvent(&orientEvent, Adafruit_BNO055::VECTOR_EULER);
  bno.getEvent(&accelEvent,  Adafruit_BNO055::VECTOR_ACCELEROMETER);
  bno.getEvent(&gyroEvent,   Adafruit_BNO055::VECTOR_GYROSCOPE);
  bno.getEvent(&magEvent,    Adafruit_BNO055::VECTOR_MAGNETOMETER);

  uint8_t sysCal, gyroCal, accelCal, magCal;
  bno.getCalibration(&sysCal, &gyroCal, &accelCal, &magCal);

  char line1[22], line2[22], line3[22], line4[22], line5[22];

  snprintf(line1, sizeof(line1), "Cal:%u/%u/%u/%u",
           sysCal, gyroCal, accelCal, magCal);
  snprintf(line2, sizeof(line2), "Ori X:%.1f Y:%.1f",
           orientEvent.orientation.x, orientEvent.orientation.y);
  snprintf(line3, sizeof(line3), "    Z:%.1f",
           orientEvent.orientation.z);
  snprintf(line4, sizeof(line4), "Acc X:%.1f Y:%.1f",
           accelEvent.acceleration.x, accelEvent.acceleration.y);
  snprintf(line5, sizeof(line5), "Mag X:%.1f Y:%.1f",
           magEvent.magnetic.x, magEvent.magnetic.y);

  display.clearBuffer();
  display.setFont(u8g2_font_5x7_tf);
  display.drawStr(0,  8, "-- BNO055 --");
  display.drawHLine(0, 10, 128);
  display.drawStr(0, 21, line1);
  display.drawStr(0, 32, line2);
  display.drawStr(0, 43, line3);
  display.drawStr(0, 54, line4);
  display.sendBuffer();

  delay(100);
}