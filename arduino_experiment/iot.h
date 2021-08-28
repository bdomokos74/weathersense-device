#include "Esp32MQTTClient.h"
#include "wifinet.h"

#ifndef IOT_H
#define IOT_H

class IotConn {
private:
  int messageCount = 0;
  bool hasIoTHub = false;
  static bool ack;
  unsigned long sendTime;
  static bool activeSession;

  static void ConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason);
  static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result);
  static void MessageCallback(const char* payLoad, int size);
  static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size);
  static int DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size);
  static void ReportConfirmationCallback(int staus);
public:
  static bool messageSending;
  IotConn(WifiNet *wifiNet, char* connectionString);
  void sendData(char* msg);
  bool messageDone();
  bool isConnected();
  bool isSendingOn();
  void close();
  void eventLoop();
  static void shellLoop();
};

#endif
