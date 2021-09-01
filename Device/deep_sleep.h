#ifndef _DEEP_SLEEP_H
#define _DEEP_SLEEP_H

#include "wifinet.h"
#include "iot.h"
#include "storage.h"
#include "state.h"
#include "led.h"
#include "esp_task_wdt.h"

#define uS_TO_S_FACTOR 1000000

class IotConn;
class DeepSleep {
private:
  WifiNet *wifiNet;
  IotConn *iotConn;
  Storage *storage;
  State *deviceState;
  LedUtil *led;

public:
  DeepSleep(WifiNet *wifiNet, IotConn *iotConn, Storage *storage, State *deviceState, LedUtil *led);
  void logWakeup(); 
  void wakeLoop();
  bool isWakeup();
  void incrementCount();
  void goSleep();
};

#endif
