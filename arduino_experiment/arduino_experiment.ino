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
//#define DO_SLEEP 1
#define SLEEP_TIME_SEC 120
#define DALLAS_PIN 15
//#define LED_PIN 13
#define LED_PIN 2
#define BME_ADDR 0x76

#define WDT_TIMEOUT 60

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
#define MEASURE_INTERVAL_MS 10000

unsigned long start_interval_ms = 0;

#define BATCH_SIZE 15
#define RTC_BUF_SIZE 3072
RTC_DATA_ATTR unsigned long startTimestamp;
RTC_DATA_ATTR int bootCnt = 0;
RTC_DATA_ATTR char dataBuf[RTC_BUF_SIZE];
RTC_DATA_ATTR char *bufPoi = dataBuf;
RTC_DATA_ATTR int numStoredMeasurements = 0;
RTC_DATA_ATTR int msgId = 0;

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
    unsigned long curr = numStoredMeasurements*SLEEP_TIME_SEC*1000+millis();
    int currTime =  curr-startTimestamp;
    Serial.print("startTimestamp= "); Serial.println(startTimestamp);
    Serial.print("curr= "); Serial.println(curr);
    Serial.print("currtime= "); Serial.println(currTime);
    dallasSensor = new DallasSensor(DALLAS_PIN);
    
    if(bmeSensor->isConnected()) {
      temp = bmeSensor->readTemp();
      pres = bmeSensor->readPressure();
      hum = bmeSensor->readHumidity();
      
    } 
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


void setup() {
  start_interval_ms = millis();
  Serial.begin(115200);
  while(!Serial) {};
  Wire.begin();
  led = new LedUtil(LED_PIN);  
  Serial.println("ESP32 Device Initializing..."); 

#ifdef DO_SLEEP
  ++bootCnt;
  Serial.println("Boot number: " + String(bootCnt));

  /* sleep code */
  esp_sleep_wakeup_cause_t reason;
  reason = esp_sleep_get_wakeup_cause();
  DeepSleep::log_wakeup(reason);
  if(bootCnt==1 || reason==ESP_SLEEP_WAKEUP_TIMER) 
  {
    int writtenChars = storeMeasurement();
    
    if(writtenChars==0) {
      // no sensors detected, flash the led and sleep
      esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * SLEEP_TIME_SEC);
      Serial.print("Elapsed ms: ");
      Serial.println((millis()-start_interval_ms));
      Serial.println("No sensors, go to sleep");
      esp_deep_sleep_start();
    }
      
    if(numStoredMeasurements < BATCH_SIZE && bootCnt>1 || numStoredMeasurements == 0) {
        led->flashLed1();
        Serial.print("Elapsed ms: ");
        Serial.println((millis()-start_interval_ms));
        Serial.println("Measurement stored: numStored/datalen:");
        Serial.println(numStoredMeasurements);
        Serial.println((int)(bufPoi-dataBuf));
        Serial.println("Go to sleep");
        esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * SLEEP_TIME_SEC);
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

    Serial.print("Elapsed ms: ");
    Serial.println((millis()-start_interval_ms));
    Serial.println("Go sleep");
    esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * SLEEP_TIME_SEC);  
    esp_deep_sleep_start();  
  }
  
#else
  bmeSensor = new BMESensor(BME_ADDR);
  dallasSensor = new DallasSensor(DALLAS_PIN);
#ifdef DO_SEVENSEG
  sevenSeg = new SevenSeg();
#endif // DO_SEVENSEG
  //wifiNet = new WifiNet(wifiSsid, wifiPw);
  //iotConn = new IotConn(wifiNet, iotConnString);

  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
#endif // DO_SLEEP
}

unsigned long lastSend = 0;
unsigned long loopCnt = 0;
void loop() {
    esp_task_wdt_reset();
  if((int)(millis() - lastSend ) > MEASURE_INTERVAL_MS ) {
    int writtenChars = storeMeasurement();

#ifdef DO_SEVENSEG 
      if(bmeSensor->isConnected()) {
        float temp = bmeSensor->readTemp();
        if(sevenSeg->isConnected()) {
          sevenSeg->print(temp);
        }
      }
#endif

    if(numStoredMeasurements < 5 && bootCnt>=0 && loopCnt < 6) {
        led->flashLed1();
        Serial.print("Elapsed ms: ");
        Serial.println((millis()-start_interval_ms));
        Serial.println("Measurement stored: numStored/datalen:");
        Serial.println(numStoredMeasurements);
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
