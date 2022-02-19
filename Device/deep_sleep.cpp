#include "deep_sleep.h"

#define LOOP_TIME_MILLIS 10000

RTC_DATA_ATTR int wakeCnt = 0;
extern bool prevConnFailed;

DeepSleep::DeepSleep(WifiNet *wifiNet, IotConn *iotConn, Storage *storage, State *deviceState, LedUtil *led) {
  this->wifiNet = wifiNet;
  this->iotConn = iotConn;
  this->storage = storage;
  this->deviceState = deviceState;
  this->led = led;
}

void DeepSleep::incrementCount() {
  wakeCnt++;
}

void DeepSleep::logWakeup() {
  esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();
  switch(reason) {
  case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", reason); break;
  }
}

bool DeepSleep::isWakeup() {
  esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();
  return (reason==ESP_SLEEP_WAKEUP_TIMER);
}

void DeepSleep::goSleep() 
{
  logMsg("go to sleep");
  ++wakeCnt;
  esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * deviceState->getSleepTimeSec());
  esp_deep_sleep_start();
}

void DeepSleep::wakeLoop() {
  logMsg("wake number: ", wakeCnt);

  int writtenChars = storage->storeMeasurement();
  if(writtenChars>0) 
  {
    led->flashLed1();
  }
  storage->printStatus();

  if(prevConnFailed) {
    Serial.println("prevconnfailed -> try wifi");
  } else 
  {
    esp_task_wdt_reset();
  }

  if(prevConnFailed || 
    storage->getNumStoredMeasurements() >= deviceState->getMeasureBatchSize() ||
    storage->isBufferFull() ||
    wakeCnt==0) 
  {
    if(storage->isBufferFull()) {
          logMsg("!buffer full - trying to send");
    }
    wifiNet->connect();  
    iotConn->connect();
    if(iotConn->isConnected())
    {
      //iotConn->subscribeTwin();
      //delay(500);
      //iotConn->requestTwinGet();
      
      esp_task_wdt_reset();
      prevConnFailed = false;
      
      logMsg("iot connected, entering loop (sleepmode)");
      unsigned long loopStartTime = millis();
      while((int)(millis()-loopStartTime)<LOOP_TIME_MILLIS)
      {
        if( storage->getNumStoredMeasurements()>0)
        {
          if(iotConn->sendData()) {
              led->flashLedSend();
              storage->reset();
          } else {
            led->flashLedErr();
          }
        }
      }
      logMsg("end loop (sleepmode)", (int)(millis()-loopStartTime));
    } else {
      logMsg("iot connect failed");
      prevConnFailed = true;
    }
  }
  
  if(deviceState->getDoSleep()) {
    logMsg("closing connections");
    iotConn->close();
    wifiNet->close(); 

    goSleep();
  }
}
