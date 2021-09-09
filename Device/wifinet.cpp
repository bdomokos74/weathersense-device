#include "wifinet.h"

#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"

#define PST_TIME_ZONE -8
#define PST_TIME_ZONE_DST_DIFF   1

#define PST_TIME_ZONE -8
#define PST_TIME_ZONE_DST_DIFF   1

#define CET_TIME_ZONE +1
#define CET_TIME_ZONE_DST_DIFF   1

#define GMT_OFFSET_SECS (CET_TIME_ZONE * 3600)
#define GMT_OFFSET_SECS_DST ((CET_TIME_ZONE + CET_TIME_ZONE_DST_DIFF) * 3600)

#define UNIX_TIME_NOV_13_2017 1510592825

#define TIME_RETRY_COUNT 5

char* wifiSsid = WIFI_SSID;
char* wifiPw = WIFI_PW;


static bool initializeTime()
{
  logMsg("Setting time using SNTP");

  configTime(GMT_OFFSET_SECS, GMT_OFFSET_SECS_DST, NTP_SERVERS);
  time_t now = time(NULL);
  int cnt = 0;
  while (now < UNIX_TIME_NOV_13_2017 && cnt < TIME_RETRY_COUNT)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  if (now < UNIX_TIME_NOV_13_2017) {
    WiFi.disconnect();
    delay(100);
    return -1;
  }
  
  Serial.print("Time initialized, ");
  Serial.println(now);
  return 0;
}

WifiNet::WifiNet() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
}
void _printState(wl_status_t state) {
  switch(state) {
    case WL_CONNECTED:
      Serial.println("WL_CONNECTED");
      break;
   case WL_NO_SHIELD:
    Serial.println("WL_NO_SHIELD");
    break;
   case WL_IDLE_STATUS:
    Serial.println("WL_IDLE_STATUS");
    break;
  case WL_NO_SSID_AVAIL:
    Serial.println("WL_NO_SSID_AVAIL");
    break;
  case WL_SCAN_COMPLETED:
    Serial.println("WL_SCAN_COMPLETED");
    break;
  case WL_CONNECT_FAILED:
    Serial.println("WL_CONNECT_FAILED");
    break;
 case WL_CONNECTION_LOST:
    Serial.println("WL_CONNECTION_LOST");
    break;
  case WL_DISCONNECTED:
    Serial.println("WL_DISCONNECTED");
    break;
  default:
    Serial.print("unknown: ");Serial.println(state);
    break;
  }
}

wl_status_t _tryConnect() {
  WiFi.persistent(false);
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid, wifiPw);
  delay(3000);
  return WiFi.status();
}

bool WifiNet::connect() {
  int retry = 10;
  wl_status_t state = _tryConnect();
  while (state != WL_CONNECTED) {
    _printState(state);
    
    if(retry==0) {
      Serial.print("\nWifi connect failed to: ");
      Serial.println(wifiSsid);
      return -1;
    }
    --retry;
    state = _tryConnect();
  }
  
  Serial.print("\nWifi Connected: ");
  Serial.println(wifiSsid);
  Serial.print("WiFi IP: ");
  Serial.println(WiFi.localIP());
  if(initializeTime()!=0)
  {
    return -1;
  }
  logTime();
}

bool WifiNet::isConnected() {

  return (WiFi.status() == WL_CONNECTED);
}


void WifiNet::close() {
  WiFi.persistent(false);
  WiFi.disconnect();
  delay(100);
}
