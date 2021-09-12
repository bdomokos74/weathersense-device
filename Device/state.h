#ifndef DSTATE_H
#define DSTATE_H

#include "Arduino.h"

#define MEASURE_INTERVAL_MS 10000
#define MEASURE_BATCH_SIZE 5
#define SLEEP_TIME_SEC 120

#define DEFAULT_LED_PIN 0

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
    bool isSevenSegTime();
    void setSevenSegTime(bool showTime);
    bool isSevenSegOn();
    void setSevenSegOn(bool ssegOn);
    bool sleepStatusChanged;

};

#endif