#include "gm_sensor.h"

#include "Wire.h"
#include "Adafruit_LiquidCrystal.h"

#define LCD_ADDR 0x20

Adafruit_LiquidCrystal lcd(LCD_ADDR);

int deviceAddr;
GMSensor::GMSensor(int addr) {
  deviceAddr = addr;
  
  lcd.begin(16, 2);
  lcd.print("init..");
  lcd.display();
  
  Serial.println("GMSensor Constructor");
  Wire.beginTransmission(deviceAddr);
  int error = Wire.endTransmission();
  if(error == 0) {
    Serial.println("GMSensor found");
    lcd.setCursor(0, 0);
    lcd.print("GMSensor found");
    found = true;
  } else {
    Serial.println("GMSensor MISSING!");
    lcd.setCursor(0, 0);
    lcd.print("GMSensor missing");
  }

}

bool GMSensor::isConnected() {
  return found;
}

unsigned long id;
unsigned long prevId;
unsigned long cpm;
unsigned long sv;
char buf1[20];
char buf2[20];
byte readBuf[8];
int GMSensor::read(byte *buf) {
  Wire.requestFrom(deviceAddr, 8);
  int i = 0;
  while(Wire.available()) {
    byte b  = Wire.read();
    Serial.print(b); Serial.print(' ');
    readBuf[i] = b;
    i++;
  }
  //Wire.endTransmission();
  Serial.println();
  
  memcpy(&id, readBuf, sizeof(long));
  memcpy(&cpm, readBuf+sizeof(long), sizeof(long));

  Serial.println("received:");
  Serial.print(id); Serial.print(' '); Serial.println(cpm);

  if(prevId!= id) {
    prevId = id;
    sv = (cpm*1000)/151;

    memcpy(buf, readBuf, 2*sizeof(long));
  
    snprintf(buf1, sizeof(buf1), "cpm: %d ->", cpm);
    snprintf(buf2, sizeof(buf2), "%d nSv/h", sv);
    
    lcd.setCursor(0, 0);
    lcd.print(buf1);
    lcd.setCursor(0, 1);
    lcd.print(buf2);
    
    return 2*sizeof(long);
  } else {
    return 0;
  }
}
