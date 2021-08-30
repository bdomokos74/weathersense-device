#include "Esp32MQTTClient.h"
#include "wifinet.h"
#include "deep_sleep.h"
#include "state.h"

#ifndef IOT_H
#define IOT_H

class IotConn {
private:
  int messageCount = 0;
  bool hasIoTHub = false;
  
  static void ConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason);
  static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result);
  static void MessageCallback(const char* payLoad, int size);
  static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size);
  static int DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size);
  static void ReportConfirmationCallback(int staus);
  static int readInt(const char* but, const char* tag);
public:

  IotConn(WifiNet *wifiNet, char* connectionString, State *deviceState);
  int sendData(char* msg);
  bool isConnected();
  bool isSendingOn();
  void close();
  int eventLoop();

  static bool activeSession;
  static bool messageSendingOn;
  static bool statusRequested;
  static bool statusAck;
  static bool sendPending;
  static bool sendFailed;
  static bool sendAck;
  static unsigned long sendTime;

};

#endif
