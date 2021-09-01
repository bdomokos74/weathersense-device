#ifndef DSTATE_H
#define DSTATE_H

#include "Arduino.h"
#include "led.h"

#define MEASURE_INTERVAL_MS 10000
#define MEASURE_BATCH_SIZE 5
#define SLEEP_TIME_SEC 120

class State {
private:
    int readInt(const char* buf, const char* tag);
    LedUtil *led;
public:
    State(LedUtil *_led);
    int getDoSleep();
    int getSleepTimeSec();
    int getMeasureIntervalMs();
    int getMeasureBatchSize();
    int getLedPin();
    void getStatusString(char* buf, int len);
    int updateState(char* payload);

    bool sleepStatusChanged;

};

#endif