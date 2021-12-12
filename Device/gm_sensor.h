#ifndef GM_SENSOR_H
#define GM_SENSOR_H

#include "Arduino.h"

class GMSensor {
private:
  bool found = false;

public:
  GMSensor(int addr);
  bool isConnected();
  int read(byte *buf);
};

#endif
