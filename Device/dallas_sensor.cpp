#include "dallas_sensor.h"

DallasSensor::DallasSensor(int oneWirePin) {
  oneWire = new OneWire(oneWirePin);
  sensors = new DallasTemperature(oneWire);
  Serial.println("DallasSensor Constructor");
  sensors->begin();
  if(sensors->getAddress(address, deviceNum)) {
    found = true;
    Serial.print("DallasSensor found: ");
    printAddress();
    Serial.println("");
  } else {
    Serial.println("DallasSensor not found.");
  }
}

void DallasSensor::printAddress() {
  for(uint8_t i  = 0; i<8; i++) {
    if(address[i] < 16) Serial.print("0");
    Serial.print(address[i], HEX);
  }
}

float DallasSensor::readTemp() {
  sensors->requestTemperatures(); 
  float tempC = sensors->getTempC(address);
  return tempC;
}

bool DallasSensor::isConnected() {
  return found;
}
