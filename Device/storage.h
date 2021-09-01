#ifndef MSTORAGE_H
#define MSTORAGE_H

#include "bme_sensor.h"
#include "dallas_sensor.h"
#include "state.h"

class Storage {
private:
  BMESensor *bmeSensor;
  DallasSensor *dallasSensor;
  State *deviceState;
public:
  Storage(BMESensor *bme, DallasSensor *dallas, State *state);
  int storeMeasurement();
  void printStatus();
  void reset();
  int getNumStoredMeasurements();
  char *getDataBuf();
  int getMeasurementString(char * buf, int size);
};

#endif