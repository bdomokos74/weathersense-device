#ifndef _WIOT_H
#define _WIOT_H

#include <mqtt_client.h>
//#include "Esp32MQTTClient.h"
#include "wifinet.h"
#include "deep_sleep.h"
#include "state.h"
#include "storage.h"

#define sizeofarray(a) (sizeof(a) / sizeof(a[0]))

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
  bool requestTwinGet();
  void close();

  static unsigned long sendTime;

};

#endif
