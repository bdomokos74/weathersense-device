#include "bme_sensor.h"

BMESensor::BMESensor() {
  Serial.println("BMESensor constructor");
  unsigned bmeStatus = bme.begin();
  if(!bmeStatus) {
    Serial.print("Did not find BME280 sensor, 0x");
    Serial.println(bme.sensorID(), 16);
    // 0xFF - bad address, 0x56-58 - BMP280, 0x60 - BME280, 0x61 - BME680
  } else {
    Serial.print("BME found: 0x");
    Serial.println(bme.sensorID(), 16);
    bmeFound = true;
  }
}

BMESensor::BMESensor(int addr) {
  Serial.println("BMESensor constructor");
  unsigned bmeStatus = bme.begin(addr);
  if(!bmeStatus) {
    Serial.print("Did not find BME280 sensor, 0x");
    Serial.println(bme.sensorID(), 16);
    // 0xFF - bad address, 0x56-58 - BMP280, 0x60 - BME280, 0x61 - BME680
  } else {
    Serial.print("BME found: 0x");
    Serial.println(bme.sensorID(), 16);
    bmeFound = true;
  }
}
bool BMESensor::isConnected() {
  return bmeFound;
}

float BMESensor::readTemp() {
  return bme.readTemperature();
}

float BMESensor::readPressure() {
  return bme.readPressure()/100.0F;
}

float BMESensor::readHumidity() {
  return bme.readHumidity();
}
