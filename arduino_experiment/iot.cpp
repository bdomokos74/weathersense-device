#include "iot.h"
#include "wifinet.h"

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */

#define MESSAGE_MAX_LEN 256
#define MESSAGE_ACK_TIMEOUT_MS 5000

#define STATUS_MSG_MAX_LEN 100
const char *statusMsg = "{\"$version\":\"%s\"}";

bool IotConn::messageSending = true;
bool IotConn::ack = false;
bool IotConn::activeSession;

void IotConn::sendData(char* msg) {
    Serial.println("sendData called with msg:");
    
    // Send teperature data
    //char messagePayload[MESSAGE_MAX_LEN];
    //snprintf(messagePayload, MESSAGE_MAX_LEN, msgTemplate, messageCount++, tempC, msg);
    Serial.println(msg);
    EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(msg, MESSAGE);
    Esp32MQTTClient_SendEventInstance(message);
    sendTime = millis();
    ack = false;
}

bool IotConn::messageDone() {
  return ack || (int)(millis()-sendTime)>MESSAGE_ACK_TIMEOUT_MS;
}

void IotConn::ConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
  Serial.print("IotConn::STATUS callback -> result: ");
  Serial.print(result);
  Serial.print(" reason: ");
  Serial.println(result);
}

void IotConn::ReportConfirmationCallback(int status)
{
  Serial.print("IotConn::ReportConfirmation callback -> status: ");
  Serial.println(status);
}

void IotConn::SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
  {
    Serial.println("Send Confirmation Callback finished.");
    ack = true;
  } else {
    Serial.print("SendConfirmationCallback failed: ");
    Serial.println(result);
  }
}

void IotConn::MessageCallback(const char* payLoad, int size)
{
  Serial.println("IotConn::Message callback:");
  Serial.println(payLoad);
  activeSession = true;
}

void IotConn::DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size)
{
  Serial.println("IotConn::DeviceTwinCallback called\nupdatestate=");
  Serial.println(updateState);
  
  char buf[STATUS_MSG_MAX_LEN];
  int n = min(size, STATUS_MSG_MAX_LEN-1);
  memcpy(buf, payLoad, n);
  buf[n] = '\0';
  // Display Twin message.
  Serial.println("payload: ");
  Serial.println(buf);

  Serial.println("reporting version START");
  
  int written = snprintf(buf, STATUS_MSG_MAX_LEN, statusMsg, "0.9");
  buf[written] = 0;
  Esp32MQTTClient_ReportState(buf);
  Serial.println("reporting version DONE");
}

int IotConn::DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
{
  Serial.println("IotConn::DeviceMethodCallback called");
  LogInfo("Try to invoke method %s", methodName);
  const char *responseMessage = "\"Successfully invoke device method\"";
  int result = 200;

  if (strcmp(methodName, "start") == 0)
  {
    LogInfo("do somethig on start");
    messageSending = true;
  }
  else if (strcmp(methodName, "stop") == 0)
  {
    LogInfo("Stop shell");
    messageSending = false;
    activeSession=false;
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

bool IotConn::isConnected() {
  return hasIoTHub;
}

bool IotConn::isSendingOn() {
  return messageSending;
}

IotConn::IotConn(WifiNet *wifiNet, char* connectionString) {
  if(!wifiNet->isConnected()) {
    Serial.println("No WiFi, skip IoT");
    hasIoTHub = false;
    return;
  }
  Serial.println("Connecting IoT");
  
  if (!Esp32MQTTClient_Init((const uint8_t*)connectionString, true))
  {
    hasIoTHub = false;
    Serial.println("Initializing IoT hub failed.");
  } else {
    hasIoTHub = true;
    activeSession = false;
    Esp32MQTTClient_SetSendConfirmationCallback(IotConn::SendConfirmationCallback);
    Esp32MQTTClient_SetMessageCallback(IotConn::MessageCallback);
    Esp32MQTTClient_SetDeviceTwinCallback(IotConn::DeviceTwinCallback);
    Esp32MQTTClient_SetDeviceMethodCallback(IotConn::DeviceMethodCallback);
    Esp32MQTTClient_SetConnectionStatusCallback(IotConn::ConnectionStatusCallback);
    Esp32MQTTClient_SetReportConfirmationCallback(IotConn::ReportConfirmationCallback);
    Serial.println("IoT Hub successfully connected");
  }
}

void IotConn::close() {
  hasIoTHub =false;
  Esp32MQTTClient_Close();
}

void IotConn::eventLoop() {
  if(activeSession) {
    shellLoop();
  } else {
    Esp32MQTTClient_Check();
    delay(100);
  }
}

void IotConn::shellLoop() {
  Serial.println("Entering shell loop...");
  
  
  while(activeSession) {
    Esp32MQTTClient_Check();
    delay(100);
  }
  Serial.println("Exiting shell loop...");
}
