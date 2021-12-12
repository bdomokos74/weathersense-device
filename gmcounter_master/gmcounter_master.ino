#include "Wire.h"
#include "Adafruit_LiquidCrystal.h"

#define LCD_ADDR 0x20
#define SLAVE_ADDR 0x25

#define READ_PERIOD 10000

//const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
Adafruit_LiquidCrystal lcd(LCD_ADDR);

int queryCmd = 1;
unsigned long previousMillis;  

int ledPin = LED_BUILTIN;
void blink() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
}

void setup(){ 

  Serial.begin(115200);
  while(!Serial) {};
  
  blink();
  
  Wire.begin();
  Wire.beginTransmission(SLAVE_ADDR);
  int error = Wire.endTransmission();
  if(error == 0) {
    Serial.println("slave found");
  } else {
    Serial.println("slave MISSING!");
  }
  
  lcd.begin(16, 2);
  lcd.print("init..");
  lcd.display();
}
unsigned long prevId = 321;
byte buf[10];
char buf1[100];
char buf2[100];
unsigned long id;
unsigned long cpm;
unsigned long sv;

void loop(){
  unsigned long currentMillis = millis();
  
  if(currentMillis - previousMillis > READ_PERIOD){
    previousMillis = currentMillis;
    Serial.println("start transfer");
    //Wire.beginTransmission(SLAVE_ADDR);
    Wire.requestFrom(SLAVE_ADDR, 8);
    int i = 0;
    while(Wire.available()) {
      byte b  = Wire.read();
      Serial.print(b); Serial.print(' ');
      buf[i] = b;
      i++;
    }
    //Wire.endTransmission();
    Serial.println();
    
    memcpy(&id, buf, sizeof(long));
    memcpy(&cpm, buf+sizeof(long), sizeof(long));

    Serial.println("received:");
    Serial.print(id); Serial.print(' '); Serial.println(cpm);

    if(prevId!= id) {
      prevId = id;
      sv = (cpm*1000)/151;

      snprintf(buf1, sizeof(buf1), "cpm: %d ->", cpm);
      snprintf(buf2, sizeof(buf2), "%d nSv/h", sv);
      
      lcd.setCursor(0, 0);
      lcd.print(buf1);
      lcd.setCursor(0, 1);
      lcd.print(buf2);
    }
  }
  
}
