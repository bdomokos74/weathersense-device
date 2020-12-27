#include "Esp32MQTTClient.h"
#include "wifinet.h"

#ifndef IOT_H
#define IOT_H

class IotConn {
private:
  int messageCount = 0;
  bool hasIoTHub = false;
  
  
  static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result);
  static void MessageCallback(const char* payLoad, int size);
  static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size);
  static int DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size);
public:
  static bool messageSending;
  IotConn(WifiNet *wifiNet, char* connectionString);
  void sendData(char* msg);
  bool isConnected();
};

#endif
