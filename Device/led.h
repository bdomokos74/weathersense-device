#ifndef LED_H
#define LED_H


class LedUtil {
private:
  void _flashLed(int num, int ms);
public:
  LedUtil();
  void flashLedErr(); 
  void flashLed();
  void flashLed1();
  void flashLedSend();
  void ledOn();
  void ledOff();
};

#endif
