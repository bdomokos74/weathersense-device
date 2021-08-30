#include "bme_sensor.h"
#include "dallas_sensor.h"

#ifndef STORAGE_H
#define STORAGE_H

class Storage {
private:
  BMESensor *bmeSensor;
  DallasSensor *dallasSensor;
public:
  Storage(BMESensor *bme, DallasSensor *dallas);
  int storeMeasurement(bool doSleep, int sleepTimeSec);
  void printStatus();
  void reset();
  int getNumStoredMeasurements();
  char *getDataBuf();
  int getMeasurementString(char * buf, int size);
};

#endif