#ifndef DALLAS_SENSOR_H
#define DALLAS_SENSOR_H

#include <OneWire.h>
#include <DallasTemperature.h>

#define tohex(v) ( ((v)<10)?'0'+(v):'A'+(v)-10 )

class DallasSensor {
private:
  bool found = false;
  int deviceNum = 0;
  DeviceAddress address;
  OneWire *oneWire;
  DallasTemperature *sensors;
public:
  DallasSensor(int oneWirePin);
  bool isConnected();
  float readTemp();
  void printAddress();
};

#endif
