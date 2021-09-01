#ifndef BMEUTIL_H
#define BMEUTIL_H

#include <Adafruit_BME280.h>

class BMESensor {
private:
  bool bmeFound = false;
  Adafruit_BME280 bme;
public:
  BMESensor();
  BMESensor(int addr);
  bool isConnected();
  float readTemp();
  float readPressure();
  float readHumidity();
};

#endif
