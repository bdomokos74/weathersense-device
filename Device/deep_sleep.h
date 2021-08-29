#include "Arduino.h"

#ifndef DEEP_SLEEP_H
#define DEEP_SLEEP_H

#define uS_TO_S_FACTOR 1000000

class DeepSleep {
private:
public:
  static void log_wakeup(esp_sleep_wakeup_cause_t reason); 
};

#endif
