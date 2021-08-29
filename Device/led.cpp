#include "Arduino.h"
#include "led.h"

LedUtil::LedUtil(int pin) {
  ledPin = pin;
}

void LedUtil::_flashLed(int num, int ms) {
  Serial.print("flashing led: "); Serial.println(ledPin);
  pinMode(ledPin, OUTPUT);
  for( int i = 0; i < num; i++) {
    digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(ms);                       // wait for a second
    digitalWrite(ledPin, LOW);
    if( i<num-1) delay(ms);
  }
}

void LedUtil::flashLedErr() {
   _flashLed(5, 200);
}
void LedUtil::flashLed() {
  _flashLed(2, 300);
}
void LedUtil::flashLed1() {
  _flashLed(1, 300);
}
void LedUtil::ledOn() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
}

void LedUtil::ledOff() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}
