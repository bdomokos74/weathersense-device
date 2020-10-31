#include <OneWire.h>
#include <DallasTemperature.h>


#include <WiFi.h>
#include <HTTPClient.h>
#include "iot_config.h"

// the setup function runs once when you press reset or power the board
const int ledPin = 14;
const int tempPin =5;
const int oneWirePin = 15;
char addr[16];
char meas[10];
String msg;

const char* ssid = WIFI_SSID;
const char* password = WIFI_PW;
const int oneWireDev = 0;
String hubName = IOT_HUB_NAME;
String devName = DEVICE_NAME;
String sasToken = SAS_TOKEN;

const char* root_ca= 
     "-----BEGIN CERTIFICATE-----\n" \
     "MIIFtDCCBJygAwIBAgIQDywQyVsGwJN/uNRJ+D6FaTANBgkqhkiG9w0BAQsFADBa\n" \
     "MQswCQYDVQQGEwJJRTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJl\n" \
     "clRydXN0MSIwIAYDVQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTE2\n" \
     "MDUyMDEyNTE1N1oXDTI0MDUyMDEyNTE1N1owgYsxCzAJBgNVBAYTAlVTMRMwEQYD\n" \
     "VQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNy\n" \
     "b3NvZnQgQ29ycG9yYXRpb24xFTATBgNVBAsTDE1pY3Jvc29mdCBJVDEeMBwGA1UE\n" \
     "AxMVTWljcm9zb2Z0IElUIFRMUyBDQSAyMIICIjANBgkqhkiG9w0BAQEFAAOCAg8A\n" \
     "MIICCgKCAgEAnqoVwRuhY1/mURjFFrsR3AtNm5EKukBJK9zWBgvFd1ksNEJFC06o\n" \
     "yRbwKPMflpW/HtOfzIeBliGk57MwZq18bgASr70sPUWuoD917HUgBfxBYoF8zA7Z\n" \
     "Ie5zAHODFboJL7Fg/apgbQs/GiZZNCi0QkQUWzw0nTUmVSNQ0mz6pCu95Dv1WMsL\n" \
     "GyPGfdN9zD3Q/QEDyJ695QgjRIxYA1DUE+54ti2k6r0ycKFQYkyWwZ25HD1h2kYt\n" \
     "3ovW85vF6y7tjTqUEcLbgKUCB81/955hdLLsbFd6f9o2PkU8xuOc3U+bUedvv6Sb\n" \
     "tvGjBEZeFyH8/CaQhzlsKMH0+OPOFv/bMqcLarPw1V1sOV1bl4W9vi2278niblzI\n" \
     "bEHt7nN888p4KNIwqCcXaGhbtS4tjn3NKI6v1d2XRyxIvCJDjgoZ09zF39Pyoe92\n" \
     "sSRikZh7xns4tQEQ8BCs4o5NBSx8UxEsgyzNSskWGEWqsIjt+7+A1skDDZv6k2o8\n" \
     "VCHNbTLFKS7d72wMI4ErpzVsBIicxaG2ezuMBBuqThxIiJ+G9zfoP9lxim/9rvJA\n" \
     "xbh3nujA1VJfkOYTJIojEAYCxR3QjEoGdapJmBle97AfqEBnwoJsu2wav8h9v+po\n" \
     "DL4h6dRzRUxY1DHypcFlXGoHu/REQgFLq2IN30/AhQLN90Pj9TT2RQECAwEAAaOC\n" \
     "AUIwggE+MB0GA1UdDgQWBBSRnjtEbD1XnEJ3KjTXT9HMSpcs2jAfBgNVHSMEGDAW\n" \
     "gBTlnVkwgkdYzKz6CFQ2hns6tQRN8DASBgNVHRMBAf8ECDAGAQH/AgEAMA4GA1Ud\n" \
     "DwEB/wQEAwIBhjAnBgNVHSUEIDAeBggrBgEFBQcDAQYIKwYBBQUHAwIGCCsGAQUF\n" \
     "BwMJMDQGCCsGAQUFBwEBBCgwJjAkBggrBgEFBQcwAYYYaHR0cDovL29jc3AuZGln\n" \
     "aWNlcnQuY29tMDoGA1UdHwQzMDEwL6AtoCuGKWh0dHA6Ly9jcmwzLmRpZ2ljZXJ0\n" \
     "LmNvbS9PbW5pcm9vdDIwMjUuY3JsMD0GA1UdIAQ2MDQwMgYEVR0gADAqMCgGCCsG\n" \
     "AQUFBwIBFhxodHRwczovL3d3dy5kaWdpY2VydC5jb20vQ1BTMA0GCSqGSIb3DQEB\n" \
     "CwUAA4IBAQBsf+pqb89rW8E0rP/cDuB9ixMX4C9OWQ7EA7n0BSllR64ZmuhU9mTV\n" \
     "2L0G4HEiGXvOmt15i99wJ0ho2/dvMxm1ZeufkAfMuEc5fQ9RE5ENgNR2UCuFB2Bt\n" \
     "bVmaKUAWxscN4GpXS4AJv+/HS0VXs5Su19J0DA8Bg+lo8ekCl4dq2G1m1WsCvFBI\n" \
     "oLIjd4neCLlGoxT2jA43lj2JpQ/SMkLkLy9DXj/JHdsqJDR5ogcij4VIX8V+bVD0\n" \
     "NCw7kQa6Ulq9Zo0jDEq1at4zSeH4mV2PMM3LwIXBA2xo5sda1cnUWJo3Pq4uMgcL\n" \
     "e0t+fCut38NMkTl8F0arflspaqUVVUov\n" \
     "-----END CERTIFICATE-----\n";

DeviceAddress tempDeviceAddress; 
int numDevices;
float val;
boolean foundOneWireDevice = false;
OneWire oneWire(oneWirePin);
DallasTemperature sensors(&oneWire);

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  sensors.begin();

  //readOneWireAddresses();
  if(sensors.getAddress(tempDeviceAddress, oneWireDev)){
    readAddress(tempDeviceAddress);
    foundOneWireDevice = true;
  }

  connectWifi();
}

// the loop function runs over and over again forever
void loop() {
  readTemp(0);
  sendData();
  flashLed();
  delay(5000);                       // wait for a second
}

//Found device 0 with address: 28AE023009000014
//Found device 1 with address: 28D37130090000E5

void readOneWireAddresses() {
  numDevices = 2;//sensors.getDeviceCount();
  Serial.print("locating devices... found ");
  Serial.print(numDevices);
  Serial.println(" devices");
  for(int i=0;i<numDevices; i++){
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i)){
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
    } else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
  }
  
}
void printTemp() {
  sensors.requestTemperatures(); 

  for(int i=0;i<numDevices; i++){
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i)){
      // Output the device ID
      Serial.print("Temperature for device: ");
      Serial.println(i,DEC);
      // Print the data
      float tempC = sensors.getTempC(tempDeviceAddress);
      Serial.print("Temp C: ");
      Serial.println(tempC);
    }
  }
}

void readTemp(int i) {
  sensors.requestTemperatures(); 
  // Search the wire for address
  if(foundOneWireDevice){
    // Output the device ID
    
    // Print the data
    float tempC = sensors.getTempC(tempDeviceAddress);
    
    String tmp = String(tempC);
    
    tmp.toCharArray(meas, tmp.length()+1);
    Serial.println("MEAS===");
    Serial.println(String(meas));
    meas[tmp.length()] = 0;
  }

}
#define tohex(v) ( ((v)<10)?'0'+(v):'A'+(v)-10 )
void readAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++){
    addr[2*i] = tohex(deviceAddress[i]>>4);
    addr[2*i+1] = tohex(deviceAddress[i]&0xf);
    Serial.println(deviceAddress[i]);
  }
}
void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++){
    if (deviceAddress[i] < 16) Serial.print("0");
      Serial.print(deviceAddress[i], HEX);
  }
}
void flashLed() {
  digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(300);                       // wait for a second
  digitalWrite(ledPin, LOW);
  delay(300);
  digitalWrite(ledPin, HIGH);
  delay(300);
  digitalWrite(ledPin, LOW);    // turn the LED off by making the voltage LOW
}

void connectWifi() {
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);

    Serial.println("Connecting to Wifi...");
  }
  Serial.println("Connected");
}

void sendData() {
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("SENDING............");
    
    msg = "{id:1234,tmp:24.69}";
    Serial.println(msg);

    Serial.println("ADDR");
    Serial.println(addr);
    Serial.println("TEMP");
    Serial.println(meas);
    //char msg[35] = ",1234,24.69,,";
    //sprintf(msg, ",%s,%s,,\n", addr, meas);
    Serial.println(msg);
    
    HTTPClient http;
    String url = "https://" + hubName + ".azure-devices.net/devices/" + devName + "/messages/events?api-version=2016-11-14";
    http.begin(url, root_ca); //Specify the URL and certificate
    http.addHeader("Authorization", sasToken);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(msg);

    if (httpCode > 0) { //Check for the returning code

        String payload = http.getString();
        Serial.println(httpCode);
        Serial.println(payload);
      }

    else {
      Serial.println("Error on HTTP request");
    }

    http.end(); //Free the resources
  }
}
