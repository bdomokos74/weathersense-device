/**
 * A simple Azure IoT example for sending telemetry to Iot Hub.
 */

#include "wifinet.h"
#include "local_config.h"
#include "iot.h"
#include "led.h"
#include "sevenseg.h"
#include "bme_sensor.h"
#include "dallas_sensor.h"
#include "storage.h"
#include "deep_sleep.h"
#include "esp_task_wdt.h"

#define DO_SEVENSEG 1

#define DALLAS_PIN 15
//#define LED_PIN 13
#define LED_PIN 2
#define BME_ADDR 0x76

#define WDT_TIMEOUT 600

char* wifiSsid = WIFI_SSID;
char* wifiPw = WIFI_PW;
char* iotConnString = DEV_CONN_STR;

BMESensor *bmeSensor;
DallasSensor *dallasSensor;
#ifdef DO_SEVENSEG
SevenSeg *sevenSeg;
#endif
WifiNet *wifiNet;
IotConn *iotConn;
LedUtil *led;
Storage *storage;

unsigned long start_interval_ms = 0;

RTC_DATA_ATTR int wakeCnt = 0;

// DEVICE state:
RTC_DATA_ATTR int doSleep = 0;
RTC_DATA_ATTR int sleepTimeSec = SLEEP_TIME_SEC;
RTC_DATA_ATTR int measureIntervalMs = MEASURE_INTERVAL_MS;
RTC_DATA_ATTR int measureBatchSize = MEASURE_BATCH_SIZE;

void wakeLoop();
void setup() {
  start_interval_ms = millis();
  Serial.begin(115200);
  while(!Serial) {};

  Wire.begin();

  led = new LedUtil(LED_PIN);  
  bmeSensor = new BMESensor(BME_ADDR);
  dallasSensor = new DallasSensor(DALLAS_PIN);
  storage = new Storage(bmeSensor, dallasSensor);

  Serial.println("ESP32 Device Initializing..."); 
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);

  esp_sleep_wakeup_cause_t reason;
  reason = esp_sleep_get_wakeup_cause();
  DeepSleep::log_wakeup(reason);
  if(doSleep && reason==ESP_SLEEP_WAKEUP_TIMER) {
    
      Serial.println("before wakeloop");
      wakeLoop();
      Serial.println("after wakeloop");
    
  } else {

#ifdef DO_SEVENSEG
  sevenSeg = new SevenSeg();
#endif // DO_SEVENSEG

  }
}

void wakeLoop() {
  Serial.print("Wake number: ");
  Serial.println(wakeCnt);

  esp_task_wdt_reset();
  int writtenChars = storage->storeMeasurement(doSleep, sleepTimeSec);
  
  if(writtenChars==0) {
    // no sensors detected, flash the led and sleep
    esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * sleepTimeSec);
    Serial.println("No sensors, go to sleep");
    ++wakeCnt;
    esp_deep_sleep_start();
  }
    
  if(storage->getNumStoredMeasurements() < measureBatchSize && wakeCnt>0 || storage->getNumStoredMeasurements() == 0) {
      led->flashLed1();
      storage->printStatus();
      
      Serial.println("Go to sleep");
      esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * sleepTimeSec);

      ++wakeCnt;
      esp_deep_sleep_start();
  }
  
  wifiNet = new WifiNet(wifiSsid, wifiPw);
  iotConn = new IotConn(wifiNet, iotConnString);

  if (iotConn->isConnected() )
  {
    Serial.print("IoTConn done, start time (ms): ");
    Serial.println(millis()-start_interval_ms);
    
    iotConn->sendData(storage->getDataBuf());

    while(!iotConn->messageDone())
    {
      Esp32MQTTClient_Check();
      delay(100);
    }

    storage->reset();

    iotConn->close();
    wifiNet->close();
    
    led->flashLed();
    Serial.println("Sending data complete");
    
  } else {
    if(!iotConn->isConnected()) {
      Serial.print("No IoT conn, (ms): ");
    } else if(!bmeSensor->isConnected()) {
      Serial.print("No BME sensor, (ms): ");
    }
    Serial.println(millis()-start_interval_ms);
    led->flashLedErr();
  }

  Serial.println("Go sleep");
  esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * sleepTimeSec);  
  ++wakeCnt;
  esp_deep_sleep_start();  
    
}

unsigned long lastSend = 0;
unsigned long loopCnt = 0;
void loop() {
    esp_task_wdt_reset();
  if((int)(millis() - lastSend ) > measureIntervalMs ) {
    int writtenChars = storage->storeMeasurement(doSleep, sleepTimeSec);

#ifdef DO_SEVENSEG 
      if(bmeSensor->isConnected()) {
        float temp = bmeSensor->readTemp();
        if(sevenSeg->isConnected()) {
          sevenSeg->print(temp);
        }
      }
#endif
    Serial.print("numStored/batchsize/wakecnt/loopcnt/loopcnt%batch =");
    Serial.print(storage->getNumStoredMeasurements());Serial.print("/");
    Serial.print(measureBatchSize);Serial.print("/");
    Serial.print(wakeCnt);Serial.print("/");
    Serial.print(loopCnt);Serial.print("/");
    Serial.println(loopCnt % measureBatchSize);
    
    if(storage->getNumStoredMeasurements() < measureBatchSize && (loopCnt % measureBatchSize)>0) {
        led->flashLed1();
        Serial.print("Elapsed ms: ");
        Serial.println((millis()-start_interval_ms));
        storage->printStatus();
    }
    else {
        wifiNet = new WifiNet(wifiSsid, wifiPw);
        iotConn = new IotConn(wifiNet, iotConnString);
        if (iotConn->isConnected() )
        {
          Serial.print("IoTConn done, start time (ms): ");
          Serial.println(millis()-start_interval_ms);

          if(storage->getNumStoredMeasurements()>0) {
            iotConn->sendData(storage->getDataBuf());

            while(!iotConn->messageDone())
            {
              iotConn->eventLoop();
            }
            led->flashLed();
            Serial.println("Sending data complete");
            
            storage->reset();
          } else {
            Serial.print("IOT - only check");
            iotConn->eventLoop();
          }
          iotConn->close();
          wifiNet->close();
          loopCnt = 0;
        } else {
          if(!iotConn->isConnected()) {
            Serial.print("No IoT conn, (ms): ");
          } else if(!bmeSensor->isConnected()) {
            Serial.print("No BME sensor, (ms): ");
          }
          Serial.println(millis()-start_interval_ms);
          led->flashLedErr();
        }
    }
    
    lastSend = millis();
    loopCnt += 1;
  } else {
    if( iotConn!=NULL && iotConn->isConnected() ) {
      iotConn->eventLoop();
    }
  }
  
  delay(100);
}
