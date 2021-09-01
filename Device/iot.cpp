#include "iot.h"
#include "led.h"
#include "sevenseg.h"
#include "storage.h"

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */

#define MESSAGE_MAX_LEN 256
#define MESSAGE_ACK_TIMEOUT_MS 10000

#define STATUS_MSG_MAX_LEN 100

char* iotConnString = DEV_CONN_STR;
volatile bool hasIoTHub = false;

extern State *deviceState;
extern LedUtil *led;
extern SevenSeg *sevenSeg;
extern Storage *storage;

static bool messageSendingOn = true;
static bool statusRequested = false;

unsigned long IotConn::sendTime = 0;

static void ConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason);
static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result);
static void MessageCallback(const char* payLoad, int size);
static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size);
static int DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size);
static void ReportConfirmationCallback(int staus);

IotConn::IotConn(WifiNet *wifiNet) 
{
  this->wifiNet = wifiNet;
}

bool IotConn::connect() 
{
  if(hasIoTHub) return true;

  if(!wifiNet->isConnected()) {
    Serial.println("No WiFi, skip IoT");
    hasIoTHub = false;
    return false;
  }
  Serial.println("Connecting IoT");
  
  if (!Esp32MQTTClient_Init((const uint8_t*)iotConnString, true, true))
  {
    hasIoTHub = false;
    Serial.println("Initializing IoT hub failed.");
  } else 
  {
    Esp32MQTTClient_SetSendConfirmationCallback(SendConfirmationCallback);
    Esp32MQTTClient_SetMessageCallback(MessageCallback);
    Esp32MQTTClient_SetDeviceTwinCallback(DeviceTwinCallback);
    Esp32MQTTClient_SetDeviceMethodCallback(DeviceMethodCallback);
    Esp32MQTTClient_SetConnectionStatusCallback(ConnectionStatusCallback);
    Esp32MQTTClient_SetReportConfirmationCallback(ReportConfirmationCallback);
    Serial.println("IoT Hub successfully connected");
    hasIoTHub = true;
  }
  return hasIoTHub;
}

bool IotConn::isConnected() 
{
  return hasIoTHub;
}

void IotConn::close() 
{
  hasIoTHub =false;
  Esp32MQTTClient_Close();
}

bool IotConn::sendData() 
{
    char *msg = storage->getDataBuf();

    Serial.println("sendData called with msg:");
    Serial.println(msg);
    //EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(msg, MESSAGE);
    return Esp32MQTTClient_SendEvent(msg);
}

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
  {
    Serial.println("SendConfirmationCallback OK");
  } else {
    Serial.print("SendConfirmationCallback failed: ");
    Serial.println(result);
  }
}

static void ConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
  // OK:result==0  FAILED:result=1
  Serial.print("IotConn::STATUS callback -> result: ");
  Serial.print(result);
  Serial.print(" reason: ");
  Serial.println(result);
  if(result==0)
    hasIoTHub = true;
  else
    hasIoTHub = false;
}

static void MessageCallback(const char* payLoad, int size)
{
  Serial.println("IotConn::Message callback:");
  Serial.println(payLoad);

}

static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size)
{
  Serial.print("IotConn::DeviceTwinCallback called\nupdatestate=");
  Serial.println(updateState);
  
  char *zeroTerminated = (char *)malloc(size + 1);
  if (zeroTerminated == NULL)
  {
    return;
  }
  memcpy(zeroTerminated, payLoad, size);
  zeroTerminated[size] = '\0';

  // Display Twin message.
  Serial.println(zeroTerminated);
  statusRequested = deviceState->updateState(zeroTerminated);
  free(zeroTerminated);
}

static int DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
{
  Serial.println("IotConn::DeviceMethodCallback called");
  LogInfo("Try to invoke method %s", methodName);

  char payloadBuf[50];
  int result = 200;
  snprintf(payloadBuf, 100, "\"OK\"");

  if (strcmp(methodName, "meas") == 0)
  {
    LogInfo("send measurements");
    storage->getMeasurementString(payloadBuf, 50);
  } else if (strcmp(methodName, "start") == 0)
  {
    LogInfo("do somethig on start");
    messageSendingOn = true;
  }
  else if (strcmp(methodName, "stop") == 0)
  {
    LogInfo("Stop shell");
    messageSendingOn = false;
  }
  else if (strcmp(methodName, "ledon") == 0)
  {
    LogInfo("led on");
    led->ledOn();
  }
  else if (strcmp(methodName, "ledoff") == 0)
  {
    LogInfo("led off");
    led->ledOff();
  }
  else if (strcmp(methodName, "7segon") == 0)
  {
    LogInfo("sevenseg on");
    if(sevenSeg!=NULL && sevenSeg->isConnected()) {
      sevenSeg->printHex(0xBEEF);
    }
  }
  else if (strcmp(methodName, "7segoff") == 0)
  {
    LogInfo("sevenseg off");
    if(sevenSeg!=NULL && sevenSeg->isConnected()) {
      sevenSeg->clear();
    }
  }
  else
  {
    LogInfo("No method %s found", methodName);
    snprintf(payloadBuf, 50, "\"FAIL\"");
    result = 404;
  }

  *response_size = strlen(payloadBuf) + 1;
  *response = (unsigned char *)strdup(payloadBuf);

  return result;
}

static void ReportConfirmationCallback(int status)
{
  Serial.print("IotConn::ReportConfirmation callback -> status: ");
  Serial.println(status);

}

bool IotConn::isSendingOn() {
  return messageSendingOn;
}

int IotConn::eventHandler() {

    if(statusRequested && hasIoTHub) {
      char buf[100];
      deviceState->getStatusString(buf, 100);
      Serial.print("Sending status: ");Serial.println(buf);
      Esp32MQTTClient_ReportState(buf);
      statusRequested = false;
    }
    
    
}

