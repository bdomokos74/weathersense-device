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
#include "state.h"
#include "log.h"
#include "esp_task_wdt.h"
#include "esp_system.h"


#define DALLAS_PIN 15

#define BME_ADDR 0x76

#define WDT_TIMEOUT 600

BMESensor *bmeSensor;
DallasSensor *dallasSensor;
SevenSeg *sevenSeg;
WifiNet *wifiNet;
IotConn *iotConn;
LedUtil *led;
Storage *storage;
State *deviceState;
DeepSleep * deepSleep;

unsigned long start_interval_ms = 0;

unsigned long lastSend = 0;
unsigned long loopCnt = 0;

RTC_DATA_ATTR bool prevConnFailed = false;


void test();
void debugState();
void show7Seg();

void setup() 
{
  Serial.begin(115200);
  while(!Serial) {};
  delay(100);
  Serial.println("ESP32 Device Initializing..."); 

  start_interval_ms = millis();
  Wire.begin();

  bmeSensor = new BMESensor(BME_ADDR);
  dallasSensor = new DallasSensor(DALLAS_PIN);
  deviceState = new State();
  led = new LedUtil();
  storage = new Storage(bmeSensor, dallasSensor, deviceState);
  wifiNet = new WifiNet();
  iotConn = new IotConn(wifiNet);
  deepSleep = new DeepSleep(wifiNet, iotConn, storage, deviceState, led);
  sevenSeg = new SevenSeg();

  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);

  test();
  
  deepSleep->logWakeup();
  if(deviceState->getDoSleep() && deepSleep->isWakeup()) {
    
      logMsg("before wakeloop");
      deepSleep->wakeLoop();
      logMsg("after wakeloop");
    
  } else 
  {
    sevenSeg->connect();

    logMsg("trying wifi connect");

    wifiNet->connect();
    iotConn->connect();

    if(iotConn->isConnected()) {
      iotConn->subscribeTwin();
      iotConn->subscribeC2D();
      iotConn->subscribeMethods();
      delay(500);
      iotConn->requestTwinGet();
    } else 
    {
      prevConnFailed = true;
      delay(deviceState->getMeasureIntervalMs());
    }
  }
  loopCnt=0;
}

void loop() 
{
  // TODO
  // else if (sasToken.IsExpired())
  // {
  //   Logger.Info("SAS token expired; reconnecting with a new one.");
  //   (void)esp_mqtt_client_destroy(mqtt_client);
  //   initializeMqttClient();
  // }

  if(loopCnt==0 || ((int)(millis() - lastSend ) > deviceState->getMeasureIntervalMs()) )
  {
    logMsg("meas loop");
    int writtenChars = storage->storeMeasurement();
    led->flashLed1();
    storage->printStatus();

    show7Seg();

    debugState();
    
    if(prevConnFailed || 
      storage->getNumStoredMeasurements() >= deviceState->getMeasureBatchSize() ||
      storage->isBufferFull()) 
    {
        if(storage->isBufferFull()) {
          logMsg("!buffer full - trying to send");
        }
        logMsg("connection loop");
        if(!wifiNet->isConnected())   
        {
          wifiNet->connect();
          iotConn->connect();
          if(iotConn->isConnected()) 
          {
            iotConn->subscribeTwin();
            iotConn->subscribeC2D();
            iotConn->subscribeMethods();
            delay(100);
            iotConn->requestTwinGet();
          }
        }
        if (iotConn->isConnected() )
        {
          prevConnFailed = false;
          esp_task_wdt_reset();

          logMsg("connected loop");

          if(  storage->getNumStoredMeasurements()>0)
          {
            if(!iotConn->requestTwinGet()) {
              logMsg("twin get req failed");
            }
            if(iotConn->sendData()) 
            {
              led->flashLedSend();
              storage->reset();
            } else {
              led->flashLedErr();
            }
          }
        } else {
          prevConnFailed = true;
        }
    }
    
    lastSend = millis();
    loopCnt += 1;
  }

  if(deviceState->getDoSleep()) {
    Serial.println("Sleep mode was requested, going to sleep...");
    iotConn->close();
    wifiNet->close();
    deepSleep->goSleep();
  }

  delay(10);
}

void debugState() 
{
    Serial.print("numStored/batchsize/loopcnt/loopcnt%batch =");
    Serial.print(storage->getNumStoredMeasurements());Serial.print("/");
    Serial.print(deviceState->getMeasureBatchSize());Serial.print("/");
    Serial.print(loopCnt);Serial.print("/");
    Serial.println(loopCnt % deviceState->getMeasureBatchSize());
}

void test() 
{
  Serial.print("esp_reset_reason()=");Serial.println(esp_reset_reason());
  Serial.print("esp_timer_get_time()=");Serial.println(esp_timer_get_time());
  Serial.print("esp_get_free_heap_size()=");Serial.println(esp_get_free_heap_size());
  Serial.print("esp_get_minimum_free_heap_size()=");Serial.println(esp_get_minimum_free_heap_size());

  char buf[100];
  if(storage->getMeasurementString(buf, 100)>0) 
  {
    Serial.println(buf);
  } else 
  {
    Serial.println("No BME!!!!!!");
  }
}

void show7Seg() 
{
  if(!sevenSeg->isConnected()) 
    {
      sevenSeg->connect();
    }
    if(bmeSensor->isConnected()) 
    {
      float temp = bmeSensor->readTemp();
      if(sevenSeg->isConnected()) 
      {
        sevenSeg->print(temp);
      }
    }
}
