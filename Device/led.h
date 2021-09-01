#ifndef LED_H
#define LED_H

class LedUtil {
private:
  int ledPin;
  void _flashLed(int num, int ms);
public:
  LedUtil(int ledPin);
  void flashLedErr(); 
  void flashLed();
  void flashLed1();
  void flashLedSend();
  void ledOn();
  void ledOff();
};

#endif
