#ifndef WIFINET_H
#define WIFINET_H

#include "local_config.h"
#include <WiFi.h>

class WifiNet {
private:
  bool hasWifi;

public:
  WifiNet();
  bool connect();
  bool isConnected();
  void close();
};

#endif
