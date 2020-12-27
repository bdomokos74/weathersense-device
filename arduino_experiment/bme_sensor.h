#include <Adafruit_BME280.h>

#ifndef BMEUTIL_H
#define BMEUTIL_H

class BMESensor {
private:
  bool bmeFound = false;
  Adafruit_BME280 bme;
public:
  BMESensor();
  bool isConnected();
  float readTemp();
  float readPressure();
  float readHumidity();
};

#endif
