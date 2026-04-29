/*
 * Feather nRF52840 Express — BLE sensor streaming test
 *
 * Streams all sensor data over BLE in three small JSON packets (~50 bytes each).
 *
 * Build:  pio run -e ble_test -t upload
 * Monitor: screen /dev/cu.usbmodem* 115200
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>
#include <bluefruit.h>
#include <SparkFun_u-blox_GNSS_v3.h>
#include <Adafruit_BME680.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

namespace {

constexpr uint8_t kGpsCs = 11;
constexpr uint8_t kBmeCs = 10;
constexpr uint8_t kUvPin = A1;

#define SERVICE_UUID  "12345678-1234-1234-1234-123456789abc"
#define SENSOR_UUID   "12345678-1234-1234-1234-123456789ab1"

BLEService        sensorService(SERVICE_UUID);
BLECharacteristic sensorChar(SENSOR_UUID);

SFE_UBLOX_GNSS_SPI gps;
Adafruit_BME680    bme(kBmeCs, &SPI);
Adafruit_BNO055    bno(55, 0x28, &Wire);

bool gpsOk = false, bmeOk = false, bnoOk = false;

float toUvIndex(int raw) {
  return (raw * (3.3f / 1023.0f)) / 0.1f;
}

void startAdv() {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(sensorService);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

void connectCallback(uint16_t conn_handle) {
  Serial.println("Phone connected!");
}

void disconnectCallback(uint16_t conn_handle, uint8_t reason) {
  Serial.println("Phone disconnected.");
}

}  // namespace

void setup() {
  pinMode(LED_RED,  OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(kGpsCs,   OUTPUT); digitalWrite(kGpsCs, HIGH);
  pinMode(kBmeCs,   OUTPUT); digitalWrite(kBmeCs, HIGH);
  pinMode(kUvPin,   INPUT);

  Serial.begin(115200);
  while (!Serial) {
    digitalWrite(LED_RED, HIGH); delay(100);
    digitalWrite(LED_RED, LOW);  delay(100);
  }

  Serial.println("Initialising sensors...");
  Wire.begin();

  gpsOk = gps.begin(SPI, kGpsCs, 5000000);
  if (gpsOk) { gps.setVal8(UBLOX_CFG_MSGOUT_UBX_NAV_PVT_SPI, 1); Serial.println("GPS OK"); }
  else Serial.println("GPS not found");

  bmeOk = bme.begin();
  if (bmeOk) {
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150);
    Serial.println("BME688 OK");
  } else Serial.println("BME688 not found");

  bnoOk = bno.begin();
  if (bnoOk) { bno.setExtCrystalUse(true); Serial.println("BNO055 OK"); }
  else Serial.println("BNO055 not found");

  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName("snowverlay");
  Bluefruit.Periph.setConnectCallback(connectCallback);
  Bluefruit.Periph.setDisconnectCallback(disconnectCallback);

  sensorService.begin();
  sensorChar.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
  sensorChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  sensorChar.setMaxLen(100);
  sensorChar.begin();

  startAdv();
  Serial.println("BLE advertising as 'snowverlay'");
  digitalWrite(LED_BLUE, HIGH);
}

void loop() {
  // ── GPS ──────────────────────────────────────────────────────────────
  double lat = 0, lon = 0;
  float  speedMph = 0;
  bool   fixValid = false;
  uint8_t hour = 0, minute = 0, month = 0, day = 0;
  uint16_t year = 0;

  if (gpsOk && gps.getPVT()) {
    fixValid  = (gps.getFixType() >= 2);
    lat       = gps.getLatitude()  / 1e7;
    lon       = gps.getLongitude() / 1e7;
    speedMph  = gps.getGroundSpeed() / 1000.0f * 2.23694f;
    bool timeValid = gps.getTimeValid() && gps.getDateValid();
    hour      = gps.getHour();
    minute    = gps.getMinute();
    year      = gps.getYear();
    month     = gps.getMonth();
    day       = gps.getDay();
  }

  // ── BME688 ───────────────────────────────────────────────────────────
  float tempC = 0, humidity = 0, pressHpa = 0, gasKOhm = 0;
  if (bmeOk && bme.performReading()) {
    tempC    = bme.temperature;
    humidity = bme.humidity;
    pressHpa = bme.pressure / 100.0f;
    gasKOhm  = bme.gas_resistance / 1000.0f;
  }

  // ── UV ───────────────────────────────────────────────────────────────
  int sum = 0;
  for (int i = 0; i < 10; i++) { sum += analogRead(kUvPin); delay(2); }
  float uvIndex = toUvIndex(sum / 10);

  // ── BNO055 ───────────────────────────────────────────────────────────
  float accelXY = 0, heading = 0;
  if (bnoOk) {
    sensors_event_t accelEvent, orientEvent;
    bno.getEvent(&accelEvent, Adafruit_BNO055::VECTOR_ACCELEROMETER);
    bno.getEvent(&orientEvent, Adafruit_BNO055::VECTOR_EULER);
    float ax = accelEvent.acceleration.x;
    float ay = accelEvent.acceleration.y;
    accelXY = sqrt(ax*ax + ay*ay);
    heading = orientEvent.orientation.x;
  }

  // ── Build 3 small packets (~50 bytes each) ────────────────────────────
  char json1[60], json2[60], json3[60];

  // Packet 1: GPS position
  snprintf(json1, sizeof(json1),
    "{\"la\":%.5f,\"lo\":%.5f,\"sp\":%.1f,\"fx\":%d}",
    lat, lon, speedMph, fixValid ? 1 : 0);

  // Packet 2: Time + temp + humidity
  snprintf(json2, sizeof(json2),
    "{\"h\":%u,\"m\":%u,\"mo\":%u,\"d\":%u,\"y\":%u,\"tm\":%.1f,\"hu\":%.1f}",
    hour, minute, month, day, year % 100, tempC, humidity);

  // Packet 3: Pressure + gas + UV + motion
  snprintf(json3, sizeof(json3),
    "{\"pr\":%.1f,\"ga\":%.1f,\"uv\":%.1f,\"ac\":%.2f,\"hd\":%.1f}",
    pressHpa, gasKOhm, uvIndex, accelXY, heading);

  if (Bluefruit.connected()) {
    digitalWrite(LED_BLUE, LOW);
    digitalWrite(LED_RED, HIGH);
    sensorChar.notify((uint8_t*)json1, strlen(json1));
    delay(100);
    sensorChar.notify((uint8_t*)json2, strlen(json2));
    delay(100);
    sensorChar.notify((uint8_t*)json3, strlen(json3));
    Serial.print("P1: "); Serial.println(json1);
    Serial.print("P2: "); Serial.println(json2);
    Serial.print("P3: "); Serial.println(json3);
    delay(50);
    digitalWrite(LED_RED, LOW);
  } else {
    digitalWrite(LED_BLUE, HIGH);
    Serial.println("Waiting for connection...");
  }

  delay(1000);
}