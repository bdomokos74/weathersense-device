#include "state.h"

RTC_DATA_ATTR int doSleep = 0;
RTC_DATA_ATTR int sleepTimeSec = SLEEP_TIME_SEC;
RTC_DATA_ATTR int measureIntervalMs = MEASURE_INTERVAL_MS;
RTC_DATA_ATTR int measureBatchSize = MEASURE_BATCH_SIZE;

const char *statusTemplate = "{\"doSleep\":\"%d\",\"sleepTimeSec\":\"%d\",\"measureIntervalMs\":\"%d\",\"measureBatchSize\":\"%d\"}";

State::State() {
    sleepStatusChanged = false;
}

int State::getDoSleep() {return doSleep;};
int State::getSleepTimeSec() {return sleepTimeSec;};
int State::getMeasureIntervalMs() {return measureIntervalMs;};
int State::getMeasureBatchSize() {return measureBatchSize;};

void State::getStatusString(char* buf, int len) {
    snprintf(buf, len, statusTemplate, doSleep, sleepTimeSec, measureIntervalMs, measureBatchSize);
}

int State::readInt(const char* buf, const char* tag) {
  char* start = strstr(buf, tag);
  if(start != NULL) {
    start += (strlen(tag));
    start += 2;
    return atoi(start);
  }
  return -1;
}
int State::updateState(char* payload) {
    bool hasChange = false;
    int val = readInt(payload, "measureIntervalMs");
    if(val != -1) {
        Serial.print("measureIntervalMs: ");Serial.print(val);
        if(measureIntervalMs!=val) {
        hasChange = true;
        Serial.print(" CHANGED from: ");Serial.println(measureIntervalMs);
        } else {
        Serial.println("");
        }
        measureIntervalMs = val;
    }
    val = readInt(payload, "measureBatchSize");
    if(val != -1) {
        Serial.print("measureBatchSize: ");Serial.print(val);
        if(measureBatchSize!=val) {
        hasChange = true;
        Serial.print(" CHANGED from: ");Serial.println(measureBatchSize);
        } else {
        Serial.println("");
        }
        measureBatchSize = val;
    }
    val = readInt(payload, "sleepTimeSec");
    if(val != -1) {
        Serial.print("sleepTimeSec: ");Serial.print(val);
        if(sleepTimeSec!=val) {
        hasChange = true;
        Serial.print(" CHANGED from: ");Serial.println(sleepTimeSec);
        } else {
        Serial.println("");
        }
        sleepTimeSec = val;
    }
    val = readInt(payload, "doSleep");
    if(val != -1) {
        Serial.print("doSleep: ");Serial.print(val);
        if(doSleep!=val) {
        hasChange = true;
        sleepStatusChanged = true;
        Serial.print(" CHANGED from: ");Serial.println(doSleep);
        } else {
        Serial.println("");
        }
        doSleep = val;
    }
    return hasChange;
}