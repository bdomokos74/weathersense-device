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

unsigned long start_interval_ms = 0;
#define DALLAS_PIN 15

BMESensor *bmeSensor;
DallasSensor *dallasSensor;
SevenSeg *sevenSeg;
WifiNet *wifiNet;
IotConn *iotConn;

// Instance params
char* wifiSsid = WIFI_SSID;
char* wifiPw = WIFI_PW;
char* iotConnString = DEV_CONN_STR;

// LED
extern int ledPin;

// deep sleep
#define uS_TO_S_FACTOR 1000000
#define SLEEP_TIME_SEC 300
RTC_DATA_ATTR int bootCnt = 0;

void log_wakeup(esp_sleep_wakeup_cause_t reason) {
  
  switch(reason) {
  case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", reason); break;
  }
}


int cnt = 0;
const char *messageTemplate = "{\"messageId\":%d,\"Temperature\":%f,\"Pressure\":%f,\"Humidity\":%f,\"bat\":%d}"; 
#define MSG_MAX_LEN 1024
#define SEND_INTERVAL_MS 60000
static uint64_t lastSend = 0;

void setup() {
  Serial.begin(115200);
  while(!Serial) {};
  
  Wire.begin();

  bmeSensor = new BMESensor();
  //dallasSensor = new DallasSensor(DALLAS_PIN);
  //sevenSeg = new SevenSeg();
  wifiNet = new WifiNet(wifiSsid, wifiPw);
  iotConn = new IotConn(wifiNet, iotConnString);
  
  ledPin = 13;
  start_interval_ms = millis();
 
  Serial.println("ESP32 Device Initializing..."); 
  ++bootCnt;
  Serial.println("Boot number: " + String(bootCnt));
  
  /* sleep code */
  esp_sleep_wakeup_cause_t reason;
  reason = esp_sleep_get_wakeup_cause();
  log_wakeup(reason);
  if(bootCnt==1 || reason==ESP_SLEEP_WAKEUP_TIMER) 
  {
    bmeSensor = new BMESensor();
    wifiNet = new WifiNet(wifiSsid, wifiPw);
    iotConn = new IotConn(wifiNet, iotConnString);
     
    if (iotConn->isConnected() && bmeSensor->isConnected())
    {
      Serial.print("IoTConn done (ms): ");
      Serial.println(millis()-start_interval_ms);

      while(! iotConn->messageSending)
      {
        Esp32MQTTClient_Check();
        delay(100);
      }
      flashLed();
      
      int battery = analogRead(A13);
      float temp = bmeSensor->readTemp();
      float pres = bmeSensor->readPressure();
      float hum = bmeSensor->readHumidity();
  
      char messagePayload[MSG_MAX_LEN];
      snprintf(messagePayload, MSG_MAX_LEN, messageTemplate, cnt++, temp, pres, hum, battery*2);
      iotConn->sendData(messagePayload);
      
    } else {
      if(!iotConn->isConnected()) {
        Serial.print("No IoT conn, (ms): ");
      } else if(!bmeSensor->isConnected()) {
        Serial.print("No BME sensor, (ms): ");
      }
      Serial.println(millis()-start_interval_ms);
      flashLedErr();
    }
    
    if(bootCnt==1 || reason==ESP_SLEEP_WAKEUP_TIMER) {
      esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * SLEEP_TIME_SEC);  
    }
  }
  
  Serial.print("Elapsed ms: ");
  Serial.println((millis()-start_interval_ms));
  Serial.println("Go sleep");
  esp_deep_sleep_start();  
}

void loop() {
  if((int)(millis() - lastSend ) > SEND_INTERVAL_MS ) {
    int battery = analogRead(A13);
    Serial.print("battery: ");
    Serial.println(battery);

    if(bmeSensor->isConnected()) {
        float temp = bmeSensor->readTemp();
        float pres = bmeSensor->readPressure();
        float hum = bmeSensor->readHumidity();
        Serial.print("BME Temp: ");
        Serial.print(temp);
        Serial.println("*C");
    
        if(sevenSeg->isConnected()) {
          sevenSeg->print(temp);
        }

        if( iotConn->isConnected() ) {
            char messagePayload[MSG_MAX_LEN];
            snprintf(messagePayload, MSG_MAX_LEN, messageTemplate, cnt++, temp, pres, hum, battery*2);
            iotConn->sendData(messagePayload);
        }
    }
    if(dallasSensor->isConnected()) {
        float dTemp = dallasSensor->readTemp();
        Serial.print("Dallas Temp:");
        Serial.print(dTemp);
        Serial.println("*C");
    }  
    lastSend = millis();
  } else {
    if( iotConn->isConnected() ) {
      Esp32MQTTClient_Check();
    }
  }
  
  delay(100);
}
