#include "Adafruit_LEDBackpack.h"

#ifndef SEVENSEG_H
#define SEVENSEG_H

class SevenSeg {
private:
  Adafruit_7segment sseg;
  bool hasSevenSeg = false;
  int sevenSegAddr;
public:
  SevenSeg();
  SevenSeg(int addr);
  bool isConnected();
  void print(int i);
  void print(float f);
  void printHex(int i);
  void clear();
  void showColon(bool c);
};

#endif
