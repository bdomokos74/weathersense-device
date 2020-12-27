#include "Arduino.h"
#include "led.h"

int ledPin;

void _flashLed(int num, int ms) {
  pinMode(ledPin, OUTPUT);
  for( int i = 0; i < num; i++) {
    digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(ms);                       // wait for a second
    digitalWrite(ledPin, LOW);
    delay(ms);
  }
}
void flashLedErr() {
   _flashLed(5, 200);
}
void flashLed() {
  _flashLed(2, 300);
}

void ledOn() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
}
void ledOff() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}
