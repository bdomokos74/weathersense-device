#ifndef _WIOT_H
#define _WIOT_H

#include "Esp32MQTTClient.h"
#include "wifinet.h"
#include "deep_sleep.h"
#include "state.h"
#include "storage.h"

class IotConn {
private:
  WifiNet *wifiNet;

  int messageCount = 0;

  static int readInt(const char* but, const char* tag);
public:

  IotConn(WifiNet *wifiNet);
  bool connect();
  bool sendData();
  bool isConnected();
  bool isSendingOn();
  void close();
  int eventHandler();


  static unsigned long sendTime;

};

#endif
