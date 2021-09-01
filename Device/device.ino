/**
 * A simple Azure IoT example for sending telemetry to Iot Hub.
 */

#define DO_SEVENSEG 1

#include "wifinet.h"
#include "local_config.h"
#include "iot.h"
#include "led.h"
#ifdef DO_SEVENSEG
#include "sevenseg.h"
#endif
#include "bme_sensor.h"
#include "dallas_sensor.h"
#include "storage.h"
#include "deep_sleep.h"
#include "state.h"
#include "esp_task_wdt.h"
#include "esp_system.h"

#define DALLAS_PIN 15
//#define DEFAULT_LED_PIN 13
#define DEFAULT_LED_PIN 2
#define BME_ADDR 0x76

#define WDT_TIMEOUT 600

BMESensor *bmeSensor;
DallasSensor *dallasSensor;
#ifdef DO_SEVENSEG
SevenSeg *sevenSeg;
#endif
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

void setup() 
{
  Serial.begin(115200);
  while(!Serial) {};

  Serial.println("ESP32 Device Initializing..."); 

  start_interval_ms = millis();
  Wire.begin();

  led = new LedUtil(DEFAULT_LED_PIN);  
  bmeSensor = new BMESensor(BME_ADDR);
  dallasSensor = new DallasSensor(DALLAS_PIN);
  deviceState = new State();
  storage = new Storage(bmeSensor, dallasSensor, deviceState);
  wifiNet = new WifiNet();
  iotConn = new IotConn(wifiNet);
  deepSleep = new DeepSleep(wifiNet, iotConn, storage, deviceState, led);
#ifdef DO_SEVENSEG
  sevenSeg = new SevenSeg();
#endif // DO_SEVENSEG

  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);

  test();
  
  deepSleep->logWakeup();
  if(deviceState->getDoSleep() && deepSleep->isWakeup()) {
    
      Serial.println("before wakeloop");
      deepSleep->wakeLoop();
      Serial.println("after wakeloop");
    
  } else 
  {
#ifdef DO_SEVENSEG
  sevenSeg->connect();
#endif // DO_SEVENSEG
    wifiNet->connect();
    iotConn->connect();
    if(!iotConn->isConnected()) {
      prevConnFailed = true;
    }
  }
}

void loop() 
{
  if(loopCnt==0 || ((int)(millis() - lastSend ) > deviceState->getMeasureIntervalMs()) )
  {
    int writtenChars = storage->storeMeasurement();
    led->flashLed1();
    storage->printStatus();

#ifdef DO_SEVENSEG 
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
#endif

    debugState();
    
    if(prevConnFailed || 
      storage->getNumStoredMeasurements() >= deviceState->getMeasureBatchSize() ) 
    {
        if(!wifiNet->isConnected())   
        {
          wifiNet->connect();
          iotConn->connect();
        }
        if (iotConn->isConnected() )
        {
          prevConnFailed = false;
          esp_task_wdt_reset();

          Serial.print("IoTConn done, start time (ms): ");
          Serial.println(millis()-start_interval_ms);

          if(storage->getNumStoredMeasurements()>0) 
          {
            if(iotConn->sendData()) 
            {
              Serial.println("Send OK");
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
  iotConn->eventHandler();
  Esp32MQTTClient_Check();

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

    Serial.print("wifi/iot: ");
    Serial.print(wifiNet->isConnected());Serial.print("/");Serial.println(iotConn->isConnected());
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

