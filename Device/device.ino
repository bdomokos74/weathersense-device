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
LedUtil * led;

const char *bmeMessageTemplate = "{\"messageId\":%d,\"Temperature\":%.2f,\"Pressure\":%.2f,\"Humidity\":%.2f,\"bat\":%.2f,\"offset\":%d}\n";
const char *dallasMessageTemplate = "{\"messageId\":%d,\"Temperature\":%.2f,\"bat\":%.2f,\"offset\":%d}\n"; 
const char *bothTemplate = "{\"Id\":%d,\"t1\":%.2f,\"p\":%.2f,\"h\":%.2f,\"bat\":%.2f,\"offset\":%d,\"t2\":%.2f}\n";

#define MSG_MAX_LEN 120

unsigned long start_interval_ms = 0;

#define RTC_BUF_SIZE 3072
RTC_DATA_ATTR unsigned long startTimestamp;
RTC_DATA_ATTR int wakeCnt = 0;
RTC_DATA_ATTR char dataBuf[RTC_BUF_SIZE];
RTC_DATA_ATTR char *bufPoi = dataBuf;
RTC_DATA_ATTR int numStoredMeasurements = 0;
RTC_DATA_ATTR int msgId = 0;

// DEVICE state:
RTC_DATA_ATTR int doSleep = 0;
RTC_DATA_ATTR int sleepTimeSec = SLEEP_TIME_SEC;
RTC_DATA_ATTR int measureIntervalMs = MEASURE_INTERVAL_MS;
RTC_DATA_ATTR int measureBatchSize = MEASURE_BATCH_SIZE;


int storeMeasurement() {
    float temp;
    float temp2;
    float pres;
    float hum;
    int bat = analogRead(A13);
    float battery = (bat*2)/4095.0F*3.3F;
    int remainingLen = RTC_BUF_SIZE-(int)(bufPoi-dataBuf);
    int writtenChars = 0;
    if(numStoredMeasurements==0) startTimestamp = millis();
    unsigned long curr = millis();
    if(doSleep && numStoredMeasurements>0) {
      curr += numStoredMeasurements*sleepTimeSec*1000;
    }
    int currTime =  curr-startTimestamp;
    Serial.print("startTimestamp/curr/currtime = "); 
    Serial.print(startTimestamp); Serial.print("/"); Serial.print(curr);
    Serial.print("/"); Serial.println(currTime);
    
    if(bmeSensor->isConnected()) {
      temp = bmeSensor->readTemp();
      pres = bmeSensor->readPressure();
      hum = bmeSensor->readHumidity();
    } 
    Serial.print("bme done");

    if(dallasSensor->isConnected()) {
        temp2 = dallasSensor->readTemp();
    }

    if(bmeSensor->isConnected()&&dallasSensor->isConnected()) {
      writtenChars = snprintf(bufPoi, remainingLen, bothTemplate, msgId++, temp, pres, hum, battery,currTime,temp2);
    } else if( bmeSensor->isConnected()) {
      writtenChars = snprintf(bufPoi, remainingLen, bmeMessageTemplate, msgId++, temp, pres, hum, battery,currTime);
    } else if (dallasSensor->isConnected()) {
       writtenChars = snprintf(bufPoi, remainingLen, dallasMessageTemplate, msgId++, temp2, battery, currTime);
    } 

   bufPoi += writtenChars;
   if(writtenChars > 0)
    numStoredMeasurements++;
  return writtenChars;
}

void wakeLoop();
void setup() {
  start_interval_ms = millis();
  Serial.begin(115200);
  while(!Serial) {};

  Wire.begin();

  led = new LedUtil(LED_PIN);  
  bmeSensor = new BMESensor(BME_ADDR);
  dallasSensor = new DallasSensor(DALLAS_PIN);

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
  int writtenChars = storeMeasurement();
  
  if(writtenChars==0) {
    // no sensors detected, flash the led and sleep
    esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * sleepTimeSec);
    Serial.println("No sensors, go to sleep");
    ++wakeCnt;
    esp_deep_sleep_start();
  }
    
  if(numStoredMeasurements < measureBatchSize && wakeCnt>0 || numStoredMeasurements == 0) {
      led->flashLed1();
      Serial.print("Measurement stored: numStored/datalen: ");
      Serial.print(numStoredMeasurements);Serial.print("/");
      Serial.println((int)(bufPoi-dataBuf));
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
    
    iotConn->sendData(dataBuf);

    while(!iotConn->messageDone())
    {
      Esp32MQTTClient_Check();
      delay(100);
    }

    led->flashLed();
    Serial.println("Sending data complete");
    bufPoi = dataBuf;
    numStoredMeasurements = 0;
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
    int writtenChars = storeMeasurement();

#ifdef DO_SEVENSEG 
      if(bmeSensor->isConnected()) {
        float temp = bmeSensor->readTemp();
        if(sevenSeg->isConnected()) {
          sevenSeg->print(temp);
        }
      }
#endif
    Serial.print("numStored/batchsize/wakecnt/loopcnt/loopcnt%batch =");
    Serial.print(numStoredMeasurements);Serial.print("/");
    Serial.print(measureBatchSize);Serial.print("/");
    Serial.print(wakeCnt);Serial.print("/");
    Serial.print(loopCnt);Serial.print("/");
    Serial.println(loopCnt % measureBatchSize);
    
    if(numStoredMeasurements < measureBatchSize && (loopCnt % measureBatchSize)>0) {
        led->flashLed1();
        Serial.print("Elapsed ms: ");
        Serial.println((millis()-start_interval_ms));
        Serial.print("Measurement stored: numStored/datalen: ");
        Serial.print(numStoredMeasurements);Serial.print("/");
        Serial.println((int)(bufPoi-dataBuf));
    }
    else {
        wifiNet = new WifiNet(wifiSsid, wifiPw);
        iotConn = new IotConn(wifiNet, iotConnString);
        if (iotConn->isConnected() )
        {
          Serial.print("IoTConn done, start time (ms): ");
          Serial.println(millis()-start_interval_ms);

          if(numStoredMeasurements>0) {
            iotConn->sendData(dataBuf);

            
            while(!iotConn->messageDone())
            {
              iotConn->eventLoop();
            }
            led->flashLed();
            Serial.println("Sending data complete");
            bufPoi = dataBuf;
            numStoredMeasurements = 0;
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
