#include "Arduino.h"
#include "led.h"

// handled  in state.cpp, persists between sleeps
extern int ledPin;

LedUtil::LedUtil() {
}

void LedUtil::_flashLed(int num, int ms) {
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
  _flashLed(1, 100);
}
void LedUtil::flashLedSend() {
  _flashLed(2, 100);
  delay(1000);
  _flashLed(2, 100);
}
void LedUtil::ledOn() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
}

void LedUtil::ledOff() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}
