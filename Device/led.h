#ifndef LED_H
#define LED_H

#define DEFAULT_LED_PIN 2
class LedUtil {
private:
  int ledPin;
  void _flashLed(int num, int ms);
public:
  LedUtil();
  void setLedPin(int ledPin);
  void flashLedErr(); 
  void flashLed();
  void flashLed1();
  void flashLedSend();
  void ledOn();
  void ledOff();
};

#endif
