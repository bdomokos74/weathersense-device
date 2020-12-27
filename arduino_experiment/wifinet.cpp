#include "wifinet.h"

WifiNet::WifiNet(char* ssid, char* password) {  
  WiFi.mode(WIFI_AP);
  WiFi.begin(ssid, password);
  int retry = 5;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    if(retry==0) {
      hasWifi = false;
      Serial.print("\nWifi connect failed: ");
      Serial.println(ssid);
      return;
    }
    --retry;
  }
  Serial.print("\nWifi Connected: ");
  Serial.println(ssid);
  Serial.print("WiFi IP: ");
  Serial.println(WiFi.localIP());
  hasWifi = true;  
}

bool WifiNet::isConnected() {
  return hasWifi;
}
