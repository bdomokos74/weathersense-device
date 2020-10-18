/**
 * A simple Azure IoT example for sending telemetry to Iot Hub.
 */

#include <WiFi.h>
#include "Esp32MQTTClient.h"
#include "iot_config.h"

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
const char *messageData = "{\"messageId\":%d, \"Temperature\":%f}";
static bool hasIoTHub = false;
static bool hasWifi = false;
int messageCount = 1;
static bool messageSending = true;
static uint64_t send_interval_ms;

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
const int ledPin = 13;

// 7seg
Adafruit_7segment sseg = Adafruit_7segment();
bool firstLoop = true;

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
    Serial.println(deviceAddress[i]);
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

void flashLed() {
  digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(300);                       // wait for a second
  digitalWrite(ledPin, LOW);
  delay(300);
  digitalWrite(ledPin, HIGH);
  delay(300);
  digitalWrite(ledPin, LOW);    // turn the LED off by making the voltage LOW
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Device");
  Serial.println("Initializing...");
  Serial.println(" > WiFi");
  Serial.println("Starting connecting WiFi.");

  delay(10);
  WiFi.mode(WIFI_AP);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    hasWifi = false;
  }
  hasWifi = true;
  
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(" > IoT Hub");
  if (!Esp32MQTTClient_Init((const uint8_t*)connectionString, true))
  {
    hasIoTHub = false;
    Serial.println("Initializing IoT hub failed.");
    return;
  }
  hasIoTHub = true;
  Esp32MQTTClient_SetSendConfirmationCallback(SendConfirmationCallback);
  Esp32MQTTClient_SetMessageCallback(MessageCallback);
  Esp32MQTTClient_SetDeviceTwinCallback(DeviceTwinCallback);
  Esp32MQTTClient_SetDeviceMethodCallback(DeviceMethodCallback);
  Serial.println("Start sending events.");
  randomSeed(analogRead(0));
  send_interval_ms = millis();

  sensors.begin();
  //readOneWireAddresses();
  if(sensors.getAddress(tempDeviceAddress, oneWireDev)){
    readAddress(tempDeviceAddress);
    foundOneWireDevice = true;
    Serial.print("Device found ");
    Serial.print(addr);
  } else {
    Serial.println("Device NOT FOUND");
  }

  pinMode(ledPin, OUTPUT);
  flashLed();

  sseg.begin(0x70);
}

void loop() {
if (hasWifi && hasIoTHub)
  {
    if ( (messageSending && 
        (int)(millis() - send_interval_ms) >= INTERVAL)
        || firstLoop)
    {
      firstLoop = false;
      // Send teperature data
      char messagePayload[MESSAGE_MAX_LEN];
      
      readTemp(0);
      snprintf(messagePayload, MESSAGE_MAX_LEN, messageData, messageCount++, tempC);
      Serial.println(messagePayload);
      EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(messagePayload, MESSAGE);
      Esp32MQTTClient_SendEventInstance(message);
      send_interval_ms = millis();
      
      sseg.println(tempC);
      sseg.writeDisplay();
      flashLed();
    }
    else
    {
      Esp32MQTTClient_Check();
    }
  }
  delay(10);
}