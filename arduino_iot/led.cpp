#include "Arduino.h"
#include "led.h"
extern int ledPin;

void _flashLed(int num, int ms) {
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
