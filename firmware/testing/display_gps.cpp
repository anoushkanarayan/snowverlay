/*
 * Feather nRF52840 Express
 *   + 1.51" transparent OLED (SSD1309, 128x64, SPI)
 *   + SparkFun NEO-M9N GPS (SPI)
 *
 * Build:  pio run -e display_gps -t upload
 */

#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <SparkFun_u-blox_GNSS_v3.h>

namespace {

constexpr uint8_t kOledCs  = 5;
constexpr uint8_t kOledDc  = 9;
constexpr uint8_t kOledRst = 6;
constexpr uint8_t kGpsCs   = 11;
constexpr int8_t  kUtcOffset = -7;  // PDT

U8G2_SSD1309_128X64_NONAME2_F_4W_HW_SPI display(U8G2_R0, kOledCs, kOledDc, kOledRst);
SFE_UBLOX_GNSS_SPI gps;

struct GpsData {
  double   lat = 0.0, lon = 0.0;
  bool     fixValid = false, timeValid = false;
  uint8_t  hour = 0, minute = 0, month = 0, day = 0;
  uint16_t year = 0;
};

GpsData gData;

void renderDisplay() {
  display.setFont(u8g2_font_5x8_tf);

  // ── Time and Date on same line, right-aligned ────────────────────────
  char topBuf[24] = "--:---- --/--/--";

  if (gData.timeValid) {
    int h     = (int)gData.hour + kUtcOffset;
    uint8_t d = gData.day;
    uint8_t m = gData.month;
    if (h < 0) { h += 24; d--; if (d == 0) { m--; d = 31; } }

    const char* ampm = (h >= 12) ? "PM" : "AM";
    if (h == 0)      h = 12;
    else if (h > 12) h -= 12;

    snprintf(topBuf, sizeof(topBuf), "%d:%02u%s %u/%02u/%02u",
             h, gData.minute, ampm, m, d, gData.year % 100);
  }

  // Right-align the whole string
  int topX = 128 - (strlen(topBuf) * 5) - 8;
  display.drawStr(topX, 8, topBuf);

  display.drawHLine(0, 10, 128);

  // ── Lat / Lon ────────────────────────────────────────────────────────
  double absLat = (gData.lat < 0) ? -gData.lat : gData.lat;
  double absLon = (gData.lon < 0) ? -gData.lon : gData.lon;
  char latDir = (gData.lat < 0) ? 'S' : 'N';
  char lonDir = (gData.lon < 0) ? 'W' : 'E';

  uint16_t latDeg  = (uint16_t)absLat;
  uint32_t latFrac = (uint32_t)((absLat - latDeg) * 100000UL);
  uint16_t lonDeg  = (uint16_t)absLon;
  uint32_t lonFrac = (uint32_t)((absLon - lonDeg) * 100000UL);

  char latLine[22], lonLine[22];
  snprintf(latLine, sizeof(latLine), "Lat: %03u.%05lu %c",
           latDeg, (unsigned long)latFrac, latDir);
  snprintf(lonLine, sizeof(lonLine), "Lon: %03u.%05lu %c",
           lonDeg, (unsigned long)lonFrac, lonDir);

  display.drawStr(0, 22, latLine);
  display.drawStr(0, 32, lonLine);

  // ── Locating overlay when no fix ─────────────────────────────────────
  if (!gData.fixValid) {
    display.setFont(u8g2_font_5x7_tf);
    display.drawStr(0, 42, "Locating...");
  }
}

}  // namespace

void setup() {
  pinMode(kGpsCs,  OUTPUT); digitalWrite(kGpsCs,  HIGH);
  pinMode(kOledCs, OUTPUT); digitalWrite(kOledCs, HIGH);

  display.begin();
  display.setContrast(255);
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tf);
  display.drawStr(0, 12, "snowverlay");
  display.drawStr(0, 28, "GPS init...");
  display.sendBuffer();

  if (!gps.begin(SPI, kGpsCs, 5000000)) {
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tf);
    display.drawStr(0, 12, "GPS not found!");
    display.drawStr(0, 28, "Check wiring.");
    display.sendBuffer();
    while (true) delay(1000);
  }

  gps.setVal8(UBLOX_CFG_MSGOUT_UBX_NAV_PVT_SPI, 1);

  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tf);
  display.drawStr(0, 12, "GPS OK");
  display.drawStr(0, 28, "Waiting for fix");
  display.sendBuffer();
}

void loop() {
  if (gps.getPVT()) {
    uint8_t fixType = gps.getFixType();
    gData.fixValid  = (fixType >= 2);
    gData.lat       = gps.getLatitude()  / 1e7;
    gData.lon       = gps.getLongitude() / 1e7;
    gData.timeValid = gps.getTimeValid() && gps.getDateValid();
    gData.hour      = gps.getHour();
    gData.minute    = gps.getMinute();
    gData.year      = gps.getYear();
    gData.month     = gps.getMonth();
    gData.day       = gps.getDay();
  }

  display.clearBuffer();
  renderDisplay();
  display.sendBuffer();
  delay(100);
}