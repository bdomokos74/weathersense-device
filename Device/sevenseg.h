#ifndef SEVENSEG_H
#define SEVENSEG_H

#include "local_config.h"

#ifdef DO_SEVENSEG
#include "Adafruit_LEDBackpack.h"
#endif
class SevenSeg {
private:
#ifdef DO_SEVENSEG
  Adafruit_7segment sseg;
#endif

  bool hasSevenSeg = false;
  int sevenSegAddr;
public:
  SevenSeg();
  SevenSeg(int addr);
  bool connect();
  bool isConnected();
  void print(int i);
  void print(float f);
  void printHex(int i);
  void clear();
  void showColon(bool c);
};

#endif
