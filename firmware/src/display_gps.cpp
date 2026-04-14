/*
 * Feather nRF52840 Express
 *   + 1.51" transparent OLED (SSD1309, 128x64, SPI)
 *   + SparkFun NEO-M9N GPS (SPI)
 *
 * Displays: Lat/Lon, Speed (mph), Altitude (m), Fix status / SIV, UTC time & date
 *
 * Build:  pio run -e display
 * Upload: pio run -e display -t upload
 *
 * Dependencies (platformio.ini lib_deps):
 *   sparkfun/SparkFun u-blox GNSS Arduino Library @ ^3
 *   olikraus/U8g2 @ ^2
 *
 * ── OLED wiring (SPI) ────────────────────────────────────────────────────────
 *   OLED VCC  -> 3V
 *   OLED GND  -> GND
 *   OLED DIN  -> MOSI
 *   OLED CLK  -> SCK
 *   OLED CS   -> kOledCs  (pin 5)
 *   OLED DC   -> kOledDc  (pin 9)
 *   OLED RST  -> kOledRst (pin 6)
 *
 * ── NEO-M9N wiring (SPI) ─────────────────────────────────────────────────────
 *   GPS VCC   -> 3V
 *   GPS GND   -> GND
 *   GPS MOSI  -> MOSI  (shared SPI bus)
 *   GPS MISO  -> MISO
 *   GPS CLK   -> SCK
 *   GPS CS    -> kGpsCs (pin 10)
 *   GPS INT   -> (optional, not used here)
 *
 * Both devices share the hardware SPI bus; CS pins keep them isolated.
 * NEO-M9N SPI max clock is 5.5 MHz — SPISettings below reflects that.
 */
 
#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <SparkFun_u-blox_GNSS_v3.h>
 
// ── Pin assignments ───────────────────────────────────────────────────────────
 
namespace {
 
constexpr uint8_t kOledCs  = 5;
constexpr uint8_t kOledDc  = 9;
constexpr uint8_t kOledRst = 6;
constexpr uint8_t kGpsCs   = 10;   // Change if you wired GPS CS elsewhere
 
// ── Peripherals ──────────────────────────────────────────────────────────────
 
U8G2_SSD1309_128X64_NONAME2_F_4W_HW_SPI display(U8G2_R0, kOledCs, kOledDc, kOledRst);
SFE_UBLOX_GNSS_SPI gps;
 
// ── State ────────────────────────────────────────────────────────────────────
 
// Cached GPS values updated each loop
struct GpsData {
  double   lat       = 0.0;
  double   lon       = 0.0;
  float    altM      = 0.0f;
  float    speedMph  = 0.0f;
  uint8_t  siv       = 0;       // satellites in view
  bool     fixValid  = false;
  uint8_t  hour      = 0;
  uint8_t  minute    = 0;
  uint8_t  second    = 0;
  uint16_t year      = 0;
  uint8_t  month     = 0;
  uint8_t  day       = 0;
};
 
GpsData gData;
 
// ── Helpers ──────────────────────────────────────────────────────────────────
 
// Formats a signed coordinate as "±DDD.DDDDD"
// buf must be at least 12 bytes
void formatCoord(char* buf, double coord) {
  char sign = (coord < 0) ? '-' : '+';
  double absVal = (coord < 0) ? -coord : coord;
  uint16_t degrees = static_cast<uint16_t>(absVal);
  uint32_t frac    = static_cast<uint32_t>((absVal - degrees) * 100000UL);
  snprintf(buf, 12, "%c%03u.%05lu", sign, degrees, static_cast<unsigned long>(frac));
}
 
// Draws all GPS fields onto the display buffer (call inside clearBuffer/sendBuffer)
void renderDisplay() {
  display.setFont(u8g2_font_5x7_tf);   // 5×7 px — fits 25 chars across 128 px
 
  // ── Row 1: Fix status & satellite count ─────────────────────────────
  char line1[22];
  snprintf(line1, sizeof(line1), "Fix:%-3s  SIV:%2u",
           gData.fixValid ? "YES" : "NO", gData.siv);
  display.drawStr(0, 7, line1);
 
  // ── Row 2: UTC time ──────────────────────────────────────────────────
  char line2[22];
  snprintf(line2, sizeof(line2), "UTC %02u:%02u:%02u  %04u-%02u-%02u",
           gData.hour, gData.minute, gData.second,
           gData.year, gData.month, gData.day);
  display.drawStr(0, 16, line2);
 
  // ── Divider ──────────────────────────────────────────────────────────
  display.drawHLine(0, 18, 128);
 
  // ── Row 3 & 4: Lat / Lon ─────────────────────────────────────────────
  char latBuf[12], lonBuf[12];
  formatCoord(latBuf, gData.lat);
  formatCoord(lonBuf, gData.lon);
 
  char line3[22], line4[22];
  snprintf(line3, sizeof(line3), "Lat: %s", latBuf);
  snprintf(line4, sizeof(line4), "Lon: %s", lonBuf);
  display.drawStr(0, 28, line3);
  display.drawStr(0, 37, line4);
 
  // ── Row 5: Altitude ──────────────────────────────────────────────────
  char line5[22];
  snprintf(line5, sizeof(line5), "Alt: %7.1f m", static_cast<double>(gData.altM));
  display.drawStr(0, 47, line5);
 
  // ── Row 6: Speed ─────────────────────────────────────────────────────
  char line6[22];
  snprintf(line6, sizeof(line6), "Spd: %6.1f mph", static_cast<double>(gData.speedMph));
  display.drawStr(0, 56, line6);
 
  // ── "No Fix" overlay when fix is absent ──────────────────────────────
  if (!gData.fixValid) {
    display.setFont(u8g2_font_ncenB10_tf);
    display.drawStr(24, 42, "Acquiring...");
    display.setFont(u8g2_font_5x7_tf);
  }
}
 
}  // namespace
 
// ── Arduino entry points ─────────────────────────────────────────────────────
 
void setup() {
  Serial.begin(115200);
  // No delay needed; GPS init will take a moment anyway
 
  // Initialise display
  display.begin();
  display.setContrast(255);
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tf);
  display.drawStr(0, 12, "snowverlay");
  display.drawStr(0, 28, "GPS init...");
  display.sendBuffer();
 
  // Initialise GPS over SPI
  // SPISettings: 5 MHz max per NEO-M9N datasheet, SPI mode 0
  if (!gps.begin(SPI, kGpsCs, 5000000)) {
    display.clearBuffer();
    display.drawStr(0, 12, "GPS not found!");
    display.drawStr(0, 28, "Check wiring.");
    display.sendBuffer();
    while (true) delay(1000);   // halt — no point continuing
  }
 
  // Request the PVT (Position Velocity Time) message automatically at 1 Hz
  gps.setVal8(UBLOX_CFG_MSGOUT_UBX_NAV_PVT_SPI, 1);
 
  display.clearBuffer();
  display.drawStr(0, 12, "GPS OK");
  display.drawStr(0, 28, "Waiting for fix");
  display.sendBuffer();
}
 
void loop() {
  // checkUblox() must be called frequently so the library can process incoming
  // SPI bytes. When a complete PVT packet is ready, getPVT() returns true.
  if (gps.getPVT()) {
    // Fix validity: fixType 3 = 3-D fix, 2 = 2-D fix
    uint8_t fixType = gps.getFixType();
    gData.fixValid  = (fixType >= 2);
    gData.siv       = gps.getSIV();
 
    // Position
    gData.lat  = gps.getLatitude()  / 1e7;   // degrees × 1e7  -> degrees
    gData.lon  = gps.getLongitude() / 1e7;
    gData.altM = gps.getAltitudeMSL() / 1000.0f;  // mm -> m
 
    // Speed: library returns mm/s; convert to mph
    gData.speedMph = gps.getGroundSpeed() / 1000.0f  // mm/s -> m/s
                     * 2.23694f;                       // m/s  -> mph
 
    // Time (UTC)
    gData.hour   = gps.getHour();
    gData.minute = gps.getMinute();
    gData.second = gps.getSecond();
    gData.year   = gps.getYear();
    gData.month  = gps.getMonth();
    gData.day    = gps.getDay();
  }
 
  // Refresh display every loop iteration (~100 ms)
  display.clearBuffer();
  renderDisplay();
  display.sendBuffer();
 
  delay(100);
}