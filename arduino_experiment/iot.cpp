#include "iot.h"
#include "wifinet.h"

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */

#define MESSAGE_MAX_LEN 256

bool IotConn::messageSending = true;

void IotConn::sendData(char* msg) {
    Serial.println("sendData called with msg:");
    
    // Send teperature data
    //char messagePayload[MESSAGE_MAX_LEN];
    //snprintf(messagePayload, MESSAGE_MAX_LEN, msgTemplate, messageCount++, tempC, msg);
    Serial.println(msg);
    EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(msg, MESSAGE);
    Esp32MQTTClient_SendEventInstance(message);
}

void IotConn::SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
  {
    Serial.println("Send Confirmation Callback finished.");
  } else {
    Serial.print("SendConfirmationCallback failed: ");
    Serial.println(result);
  }
}

void IotConn::MessageCallback(const char* payLoad, int size)
{
  Serial.println("IotConn::Message callback:");
  Serial.println(payLoad);
}

void IotConn::DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size)
{
  Serial.println("IotConn::DeviceTwinCallback called");
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

int IotConn::DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
{
  Serial.println("IotConn::DeviceMethodCallback called");
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

bool IotConn::isConnected() {
  return hasIoTHub;
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
    Esp32MQTTClient_SetSendConfirmationCallback(IotConn::SendConfirmationCallback);
    Esp32MQTTClient_SetMessageCallback(IotConn::MessageCallback);
    Esp32MQTTClient_SetDeviceTwinCallback(IotConn::DeviceTwinCallback);
    Esp32MQTTClient_SetDeviceMethodCallback(IotConn::DeviceMethodCallback);
    Serial.println("IoT Hub successfully connected");
  }
}
