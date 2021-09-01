#include "deep_sleep.h"

#define LOOP_TIME_MILLIS 10000

extern unsigned long start_interval_ms;
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
  Serial.println("Go to sleep");
  ++wakeCnt;
  esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * deviceState->getSleepTimeSec());
  esp_deep_sleep_start();
}

void DeepSleep::wakeLoop() {
  Serial.print("Wake number: ");
  Serial.println(wakeCnt);

  int writtenChars = storage->storeMeasurement();
  if(writtenChars>0) 
  {
    led->flashLed1();
  }
  storage->printStatus();

  if(prevConnFailed) {
    Serial.println("prevconnfailed -> try wifi");
  }

  if(prevConnFailed || storage->getNumStoredMeasurements() >= deviceState->getMeasureBatchSize() 
    || wakeCnt==0) 
  {
    wifiNet->connect();
    if(wifiNet->isConnected()) 
    {
      iotConn->connect();
    }
    
    if(iotConn->isConnected()) 
    {
      esp_task_wdt_reset();

      Serial.print("iot connected (ms): ");
      Serial.println((int)(millis()-start_interval_ms));
      prevConnFailed = false;

      unsigned long loopStartTime = millis();
      Serial.print("start loop");
      while((int)(millis()-loopStartTime)<LOOP_TIME_MILLIS)
      {
        if (storage->getNumStoredMeasurements()>0)
        {
          if(iotConn->sendData()) {
              led->flashLedSend();
              storage->reset();
          } else {
            led->flashLedErr();
          }
        }
        iotConn->eventHandler();
        Esp32MQTTClient_Check();
      }
      Serial.print("end loop: ");Serial.println((int)(millis()-loopStartTime));
    } else {
      Serial.print("iot connect failed (ms): ");
      Serial.println((int)(millis()-start_interval_ms));
      prevConnFailed = true;
    }
  }
  
  if(deviceState->getDoSleep()) {
    iotConn->close();
    wifiNet->close(); 

    goSleep();
  }
}

