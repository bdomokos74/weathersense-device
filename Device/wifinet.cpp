#include "wifinet.h"

char* wifiSsid = WIFI_SSID;
char* wifiPw = WIFI_PW;

WifiNet::WifiNet() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
}

bool WifiNet::connect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid, wifiPw);
  int retry = 10;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    if(retry==0) {
      hasWifi = false;
      Serial.print("\nWifi connect failed to: ");
      Serial.println(wifiSsid);
      return -1;
    }
    --retry;
  }
  Serial.print("\nWifi Connected: ");
  Serial.println(wifiSsid);
  Serial.print("WiFi IP: ");
  Serial.println(WiFi.localIP());
  hasWifi = true;  
}

bool WifiNet::isConnected() {
  return hasWifi;
}


void WifiNet::close() {
  WiFi.disconnect();
  delay(100);
  hasWifi = false;
}
