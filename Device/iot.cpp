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

extern State *deviceState;
extern LedUtil *led;
extern SevenSeg *sevenSeg;
extern Storage *storage;

bool IotConn::messageSendingOn = true;

bool IotConn::activeSession = false;
bool IotConn::statusRequested = false;
bool IotConn::statusAck = false;
bool IotConn::sendPending = false;
bool IotConn::sendFailed = false;
bool IotConn::sendAck = false;
unsigned long IotConn::sendTime = 0;

IotConn::IotConn(WifiNet *wifiNet, char* connectionString, State* deviceState) {

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

int IotConn::sendData(char* msg) {
    
    Serial.println("sendData called with msg:");
    Serial.println(msg);
    //EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(msg, MESSAGE);
    if(!Esp32MQTTClient_SendEvent(msg)) {
      return -1;
    }
    sendPending = true;
    sendTime = millis();
    eventLoop();
    return 0;
}

void IotConn::SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
  {
    Serial.println("Send Confirmation Callback finished.");
    sendAck = true;
    Serial.print("after sendAck set: "); Serial.println(IotConn::sendAck);
  } else {
    Serial.print("SendConfirmationCallback failed: ");
    Serial.println(result);
    sendFailed = true;
  }
}


void IotConn::ConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
  Serial.print("IotConn::STATUS callback -> result: ");
  Serial.print(result);
  Serial.print(" reason: ");
  Serial.println(result);
}

void IotConn::MessageCallback(const char* payLoad, int size)
{
  Serial.println("IotConn::Message callback:");
  Serial.println(payLoad);
  if(strncmp(payLoad, "cmd", 3)==0) {
    Serial.println("running shell");
    activeSession = true;
  }
}

void IotConn::DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size)
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

int IotConn::DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
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
  else if (strcmp(methodName, "exit") == 0)
  {
    activeSession=false;
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

bool IotConn::isConnected() {
  return hasIoTHub;
}

bool IotConn::isSendingOn() {
  return messageSendingOn;
}

void IotConn::close() {
  hasIoTHub =false;
  Esp32MQTTClient_Close();
}


int IotConn::eventLoop() {
  bool error = false;
  Esp32MQTTClient_Check();
  while(
    statusRequested||
    sendPending||
    activeSession
   ) {
    if(statusRequested) {
      char buf[100];
      deviceState->getStatusString(buf, 100);
      Serial.print("Sending status: ");Serial.println(buf);
      Esp32MQTTClient_ReportState(buf);
      statusRequested = false;
    }
    if(sendPending) {
      if(sendAck) {
        sendPending = false;
      }
      if(sendFailed) {
        sendPending = false;
        error = true;
        Serial.println("Send failed");
      }
      if((int)(millis()-sendTime)>MESSAGE_ACK_TIMEOUT_MS) {
        sendPending = false;
        error = true;
        Serial.println("Send timeout");
      }
    }
    Esp32MQTTClient_Check();
    delay(50);
  }
  return error;
}
extern int wakeCnt;
void IotConn::ReportConfirmationCallback(int status)
{
  Serial.print("IotConn::ReportConfirmation callback -> status: ");
  Serial.println(status);

  if(deviceState->sleepStatusChanged) {
    deviceState->sleepStatusChanged = false;
    wakeCnt = 1;
    Serial.println("doSleep requested, going to sleep");
    esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * deviceState->getSleepTimeSec());
    esp_deep_sleep_start();
  }
}
