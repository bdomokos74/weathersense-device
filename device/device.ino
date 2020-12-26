/**
 * A simple Azure IoT example for sending telemetry to Iot Hub.
 */

#include <WiFi.h>
#include "Esp32MQTTClient.h"
#include "iot_config.h"

#include "led.h"

#include <OneWire.h>
#include <DallasTemperature.h>

#include "Adafruit_LEDBackpack.h"

#define INTERVAL 120000
#define MESSAGE_MAX_LEN 256
// Please input the SSID and password of WiFi
const char* ssid     = WIFI_SSID;
const char* password = WIFI_PW;

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */
static const char* connectionString = HUZZAH32_DEV_CONN_STR;
const char *messageData = "{\"messageId\":%d, \"Temperature\":%f, \"devmsg\":%s}";
const char *devData = "{\"bootCnt\":%d, \"wa_reason\":%d}";
static bool hasIoTHub = false;
static bool hasWifi = false;
int messageCount = 1;
static bool messageSending = true;
unsigned long start_interval_ms = 0;

// 1wire
const int oneWireDev = 0;
char addr[16];
const int tempPin =5;
const int oneWirePin = 15;
DeviceAddress tempDeviceAddress; 
int numDevices;
float tempC;
boolean foundOneWireDevice = false;
OneWire oneWire(oneWirePin);
DallasTemperature sensors(&oneWire);

// LED
int ledPin = 13;

// 7seg
// Connection:
// + -> USB
// - -> GND
// D -> SDA
// C -> SCL
Adafruit_7segment sseg = Adafruit_7segment();
static bool hasSevenSeg = false;
int sevenSegAddr = 0x70;

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

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
  {
    Serial.println("Send Confirmation Callback finished.");
  }
}

static void MessageCallback(const char* payLoad, int size)
{
  Serial.println("Message callback:");
  Serial.println(payLoad);
}

static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size)
{
  char *temp = (char *)malloc(size + 1);
  if (temp == NULL)
  {
    return;
  }
  memcpy(temp, payLoad, size);
  temp[size] = '\0';
  // Display Twin message.
  Serial.println(temp);
  free(temp);
}

static int  DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
{
  LogInfo("Try to invoke method %s", methodName);
  const char *responseMessage = "\"Successfully invoke device method\"";
  int result = 200;

  if (strcmp(methodName, "start") == 0)
  {
    LogInfo("Start sending temperature and humidity data");
    messageSending = true;
  }
  else if (strcmp(methodName, "stop") == 0)
  {
    LogInfo("Stop sending temperature and humidity data");
    messageSending = false;
  }
  else
  {
    LogInfo("No method %s found", methodName);
    responseMessage = "\"No method found\"";
    result = 404;
  }

  *response_size = strlen(responseMessage) + 1;
  *response = (unsigned char *)strdup(responseMessage);

  return result;
}

// One Wire
#define tohex(v) ( ((v)<10)?'0'+(v):'A'+(v)-10 )
void readAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++){
    addr[2*i] = tohex(deviceAddress[i]>>4);
    addr[2*i+1] = tohex(deviceAddress[i]&0xf);
  }
}
void readTemp(int i) {
  sensors.requestTemperatures(); 
  // Search the wire for address
  if(foundOneWireDevice){
    // Output the device ID
    
    // Print the data
    tempC = sensors.getTempC(tempDeviceAddress);
    Serial.println("TempC="+String(tempC));
  }
}
void initWifi() {
  delay(10);
  WiFi.mode(WIFI_AP);
  WiFi.begin(ssid, password);
  int retry = 10;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    if(retry==0) {
      hasWifi = false;
      return;
    }
    --retry;
  }
  hasWifi = true;  
}
void initHub() {
  Serial.println("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());
  Serial.println(" > IoT Hub");
  if (!Esp32MQTTClient_Init((const uint8_t*)connectionString, true))
  {
    hasIoTHub = false;
    Serial.println("Initializing IoT hub failed.");
  } else {
    hasIoTHub = true;
  }
}
void initSevenSeg() {
  Wire.beginTransmission(sevenSegAddr);
  int errorResult = Wire.endTransmission();
  if(errorResult==0) {
    Serial.println("Found seven seg at 0x70");
    sseg.begin(sevenSegAddr);
    hasSevenSeg = true;
  } else {
    Serial.println("No seven seg found");
  }
}

void sendData(char* msg) {

    // Send teperature data
    char messagePayload[MESSAGE_MAX_LEN];
    snprintf(messagePayload, MESSAGE_MAX_LEN, messageData, messageCount++, tempC, msg);
    Serial.println(messagePayload);
    EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(messagePayload, MESSAGE);
    Esp32MQTTClient_SendEventInstance(message);

}

void run(char* msg) {
  
  if (hasWifi && hasIoTHub)
  {
    if ( messageSending)
    {
      
      flashLed();
      
      readTemp(0);
      
      sendData(msg);
      
      if(hasSevenSeg) {
        sseg.println(tempC);
        sseg.writeDisplay();
      }
    }
    else
    {
      Esp32MQTTClient_Check();
    }
  } else {
    flashLedErr();
  }
  
}

void setup() {
  start_interval_ms = millis();
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 Device Initializing...");
  
  ++bootCnt;
  Serial.println("Boot number: " + String(bootCnt));
  esp_sleep_wakeup_cause_t reason;
  reason = esp_sleep_get_wakeup_cause();
  log_wakeup(reason);
  if(bootCnt==1 || reason==ESP_SLEEP_WAKEUP_TIMER) 
  {
    Serial.println("Starting connecting WiFi.");
    initWifi();
    
    if(hasWifi) {
      Serial.print("Wifi done (ms): ");
      Serial.println(millis()-start_interval_ms);
      initHub();
    
      Serial.print("IoTHUB done (ms): ");
      Serial.println(millis()-start_interval_ms);
      Wire.begin();
      initSevenSeg();
      
      Esp32MQTTClient_SetSendConfirmationCallback(SendConfirmationCallback);
      Esp32MQTTClient_SetMessageCallback(MessageCallback);
      Esp32MQTTClient_SetDeviceTwinCallback(DeviceTwinCallback);
      Esp32MQTTClient_SetDeviceMethodCallback(DeviceMethodCallback);
      Serial.println("Start sending events.");
      randomSeed(analogRead(0));
      
      sensors.begin();
      //readOneWireAddresses();
      if(sensors.getAddress(tempDeviceAddress, oneWireDev)){
        readAddress(tempDeviceAddress);
        foundOneWireDevice = true;
        Serial.print("Device found ");
        Serial.println(addr);
      } else {
        Serial.println("Device NOT FOUND");
      }
    
      pinMode(ledPin, OUTPUT);
      
      Serial.print("OneWire done (ms): ");
      Serial.println(millis()-start_interval_ms);

      char devMsg[MESSAGE_MAX_LEN];
      snprintf(devMsg, MESSAGE_MAX_LEN, devData, bootCnt, (int)reason);
      
      run(devMsg);
      
    } else {
      Serial.print("No wifi! (ms): ");
      Serial.println(millis()-start_interval_ms);
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
  //delay(10);
}
