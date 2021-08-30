#include "Arduino.h"

#ifndef STATE_H
#define STATE_H

#define MEASURE_INTERVAL_MS 10000
#define MEASURE_BATCH_SIZE 5
#define SLEEP_TIME_SEC 120

class State {
private:
    int readInt(const char* buf, const char* tag);
public:
    State();
    int getDoSleep();
    int getSleepTimeSec();
    int getMeasureIntervalMs();
    int getMeasureBatchSize();

    void getStatusString(char* buf, int len);
    int updateState(char* payload);

    bool sleepStatusChanged;

};

#endif