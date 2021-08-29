#include "local_config.h"
#include <WiFi.h>

#ifndef WIFINET_H
#define WIFINET_H

class WifiNet {
private:
  bool hasWifi;
public:
  WifiNet(char* ssid, char* pw);
  bool isConnected();
  void close();
};

#endif
