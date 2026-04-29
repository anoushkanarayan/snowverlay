/*
 * Feather nRF52840 Express — NEO-M9N GPS SPI terminal test
 *
 * Build:  pio run -e gps_test -t upload
 * Monitor: screen /dev/cu.usbmodem1101 115200
 *
 * ── NEO-M9N wiring (SPI) ─────────────────────────────────────────────────────
 *   GPS VCC   -> 3V
 *   GPS GND   -> GND
 *   GPS MOSI  -> MOSI
 *   GPS MISO  -> MISO
 *   GPS CLK   -> SCK
 *   GPS CS    -> kGpsCs (pin 11)
 */

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <SPI.h>
#include <SparkFun_u-blox_GNSS_v3.h>

namespace {
  constexpr uint8_t kGpsCs = 11;
  SFE_UBLOX_GNSS_SPI gps;
}

void setup() {
  pinMode(LED_RED, OUTPUT);

  // Force other SPI CS pins HIGH
  pinMode(5, OUTPUT);   // OLED CS
  digitalWrite(5, HIGH);
  pinMode(10, OUTPUT);  // BME688 CS
  digitalWrite(10, HIGH);

  Serial.begin(115200);
  while (!Serial) {
    digitalWrite(LED_RED, HIGH);
    delay(100);
    digitalWrite(LED_RED, LOW);
    delay(100);
  }

  Serial.println("──────────────────────────────────────");
  Serial.println("  NEO-M9N GPS SPI Test");
  Serial.println("──────────────────────────────────────");
  Serial.println("Initialising GPS...");

  if (!gps.begin(SPI, kGpsCs, 5000000)) {
    Serial.println("ERROR: GPS not detected. Check wiring and CS pin.");
    while (true) {
      digitalWrite(LED_RED, HIGH);
      delay(100);
      digitalWrite(LED_RED, LOW);
      delay(100);
    }
  }

  gps.setVal8(UBLOX_CFG_MSGOUT_UBX_NAV_PVT_SPI, 1);
  Serial.println("GPS OK — waiting for data...\n");
}

void loop() {
  if (!gps.getPVT()) {
    delay(50);
    return;
  }

  uint8_t fixType = gps.getFixType();
  const char* fixStr = "None";
  if      (fixType == 1) fixStr = "Dead reckoning";
  else if (fixType == 2) fixStr = "2D";
  else if (fixType == 3) fixStr = "3D";
  else if (fixType == 4) fixStr = "GNSS+DR";
  else if (fixType == 5) fixStr = "Time only";

  double lat      = gps.getLatitude()     / 1e7;
  double lon      = gps.getLongitude()    / 1e7;
  double altM     = gps.getAltitudeMSL()  / 1000.0;
  double altFt    = altM * 3.28084;
  double speedMs  = gps.getGroundSpeed()  / 1000.0;
  double speedMph = speedMs * 2.23694;
  double speedKph = speedMs * 3.6;
  double heading  = gps.getHeading()      / 1e5;
  double hAccM    = gps.getHorizontalAccEst() / 1000.0;
  double vAccM    = gps.getVerticalAccEst()   / 1000.0;

  uint16_t year   = gps.getYear();
  uint8_t  month  = gps.getMonth();
  uint8_t  day    = gps.getDay();
  uint8_t  hour   = gps.getHour();
  uint8_t  minute = gps.getMinute();
  uint8_t  second = gps.getSecond();
  uint16_t ms     = gps.getMillisecond();
  bool     timeValid = gps.getTimeValid() && gps.getDateValid();

  uint8_t siv = gps.getSIV();

  Serial.println("──────────────────────────────────────");
  Serial.print("Fix type    : "); Serial.print(fixType);
  Serial.print(" ("); Serial.print(fixStr); Serial.println(")");
  Serial.print("Satellites  : "); Serial.println(siv);
  Serial.println();

  if (timeValid) {
    char timeBuf[28];
    snprintf(timeBuf, sizeof(timeBuf), "%04u-%02u-%02u %02u:%02u:%02u.%03u",
             year, month, day, hour, minute, second, ms);
    Serial.print("UTC time    : "); Serial.print(timeBuf); Serial.println(" UTC");
  } else {
    Serial.println("UTC time    : (not yet valid)");
  }

  Serial.println();
  Serial.print("Latitude    : "); Serial.print(lat,  7); Serial.println(" °");
  Serial.print("Longitude   : "); Serial.print(lon,  7); Serial.println(" °");
  Serial.print("Altitude    : "); Serial.print(altM, 2); Serial.print(" m  /  ");
                                  Serial.print(altFt, 1); Serial.println(" ft");
  Serial.println();
  Serial.print("Speed       : "); Serial.print(speedMph, 2); Serial.print(" mph  /  ");
                                  Serial.print(speedKph,  2); Serial.println(" kph");
  Serial.print("Heading     : "); Serial.print(heading, 2); Serial.println(" °");
  Serial.println();
  Serial.print("Horiz. acc  : ±"); Serial.print(hAccM, 2); Serial.println(" m");
  Serial.print("Vert.  acc  : ±"); Serial.print(vAccM, 2); Serial.println(" m");
  Serial.println();

  delay(1000);
}