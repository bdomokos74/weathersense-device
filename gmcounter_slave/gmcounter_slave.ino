/*
 * Geiger counter Kit could get on:  https://www.aliexpress.com            search: geiger counter kit
* --------------------------------------------------------------------------------------
* WHAT IS CPM?
* CPM (or counts per minute) is events quantity from Geiger Tube you get during one minute. Usually it used to 
* calculate a radiation level. Different GM Tubes has different quantity of CPM for background. Some tubes can produce
* about 10-50 CPM for normal background, other GM Tube models produce 50-100 CPM or 0-5 CPM for same radiation level.
* Please refer your GM Tube datasheet for more information. Just for reference here, J305 and SBM-20 can generate 
* about 10-50 CPM for normal background. 
* --------------------------------------------------------------------------------------
* HOW TO CONNECT GEIGER KIT?
* The kit 3 wires that should be connected to Arduino UNO board: 5V, GND and INT. PullUp resistor is included on
* kit PCB. Connect INT wire to Digital Pin#2 (INT0), 5V to 5V, GND to GND. Then connect the Arduino with
* USB cable to the computer and upload this sketch. 
* 
 * Author:JiangJie Zhang * If you have any questions, please connect cajoetech@qq.com
 * 
 * License: MIT License
 * 
 * Please use freely with attribution. Thank you!
*/
// Slave I2C node:
// Board: Arduino Nano, ATmega328P, /dev/ttyUSB1
// Pins:
// Dat: (brown) A4
// Clk: (blue) A5
// GND, 5V
// INT: D2

// Board: DOIT ESP32 DEVKIT v1
// Pins:
// SDA: GPIO21
// SCL: GPIO22

#include "Wire.h"
#include "Adafruit_LiquidCrystal.h"

#define LOG_PERIOD 30000  //Logging period in milliseconds, recommended value 15000-60000.
#define MAX_PERIOD 60000  //Maximum logging period without modifying this sketch

unsigned long counts;     //variable for GM Tube events
unsigned long curr;
unsigned long id;
unsigned long countsTmp;
unsigned long sv;
unsigned long cpm;        //variable for CPM
unsigned int multiplier;  //variable for calculation CPM in this sketch
unsigned long previousMillis;  //variable for time measurement

int arrival[200];
unsigned long prev = 0;
int interruptPin = 2;

#define LCD_ADDR 0x20
#define MY_ADDR 0x25

//IRAM_ATTR 
void  tube_impulse(){       //subprocedure for capturing events from Geiger Kit
  //Serial.print(".");
  curr = millis();
  arrival[counts%sizeof(arrival)] = (int)(curr-prev);
  counts++;
  prev = curr;
}

byte rcvBuf[10];
byte sendBuf[10];

//const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
//Adafruit_LiquidCrystal lcd(LCD_ADDR);

void handleQuery(int receivedBytes) {
  for(int i = 0; i<receivedBytes; i++) {
    rcvBuf[i] = Wire.read();
  }
  if(rcvBuf[0]==0x01) {
     Wire.write(sendBuf, sizeof(int)+sizeof(long));
  }
}

void setup(){             //setup subprocedure
  counts = 0;
  cpm = 0;
  multiplier = MAX_PERIOD / LOG_PERIOD;      //calculating multiplier, depend on your log period
  Serial.begin(115200);
  while(!Serial) {};

  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), tube_impulse, FALLING); //define external interrupts 

  Wire.begin(MY_ADDR);
  Wire.onReceive(handleQuery);
  Serial.println("start");
  
  //lcd.begin(16, 2);
  //lcd.print("starting..");
  //lcd.display();
}

void loop(){                                 //main cycle
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > LOG_PERIOD){
    
    previousMillis = currentMillis;
    
    noInterrupts();
    countsTmp = counts;
    cpm = countsTmp * multiplier;
    counts = 0;
    id = curr;
    memcpy(sendBuf, &id, sizeof(unsigned long));
    memcpy(sendBuf+sizeof(unsigned long), &cpm, sizeof(unsigned long));
    interrupts();
    
    sv = (cpm*1000)/151;
    
    //unsigned long sv = (cpm*1000)/151;
    char buf1[100];
    char buf2[100];
    snprintf(buf1, sizeof(buf1), "cpm: %d ->", cpm);
    snprintf(buf2, sizeof(buf2), "%d nSv/h", sv);
    
    Serial.println();
    Serial.println(buf1);
    Serial.println(buf2);
    Serial.println(curr);

    unsigned long newid;
    unsigned long newcnt;
    memcpy(&newid, sendBuf, sizeof(long));
    memcpy(&newcnt, sendBuf+sizeof(long), sizeof(long));

    Serial.print(newid); Serial.print(' '); Serial.println(newcnt);
    
    
    //lcd.setCursor(0, 0);
    //lcd.print(buf1);
    //lcd.setCursor(0, 1);
    //lcd.print(buf2);
    
    
  }
  
}
