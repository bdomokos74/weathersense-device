#include "sevenseg.h"

// 7seg
// Connection:
// + -> USB
// - -> GND
// D -> SDA
// C -> SCL

SevenSeg::SevenSeg(int addr) 
{
  sevenSegAddr = addr;
}

bool SevenSeg::connect() 
{
  #ifdef DO_SEVENSEG
  // Wire.begin(); // NEEDED, put to setup()
  Wire.beginTransmission(sevenSegAddr);
  int errorResult = Wire.endTransmission();
  if(errorResult==0) {
    Serial.print("Found seven seg at 0x");
    Serial.println(sevenSegAddr, 16);
    sseg.begin(sevenSegAddr);
    hasSevenSeg = true;
  } else {
    Serial.println("No seven seg found");
    hasSevenSeg = false;
  }
  #else
    hasSevenSeg = false;
    return false;
  #endif
}

SevenSeg::SevenSeg(): SevenSeg(0x70) {}

bool SevenSeg::isConnected() {
  return hasSevenSeg;
}

void SevenSeg::print(int i) {
  #ifdef DO_SEVENSEG
  sseg.println(i);
  sseg.writeDisplay();
  #endif
}
void SevenSeg::printHex(int i) {
  #ifdef DO_SEVENSEG
  sseg.print(i, HEX);
  sseg.writeDisplay();
  #endif
}

void SevenSeg::print(float f) {
  #ifdef DO_SEVENSEG
  sseg.println(f);
  sseg.writeDisplay();
  #endif
}
void SevenSeg::clear() {
  #ifdef DO_SEVENSEG
  sseg.clear();
  sseg.writeDisplay();
  #endif
}

void SevenSeg::showColon(bool col) {
  #ifdef DO_SEVENSEG
  sseg.drawColon(col);
  sseg.writeDisplay();
  #endif
}
