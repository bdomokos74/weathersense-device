#include "sevenseg.h"

// 7seg
// Connection:
// + -> USB
// - -> GND
// D -> SDA
// C -> SCL

SevenSeg::SevenSeg(int addr) {
  sevenSegAddr = addr;
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
}

SevenSeg::SevenSeg(): SevenSeg(0x70) {}

bool SevenSeg::isConnected() {
  return hasSevenSeg;
}

void SevenSeg::print(int i) {
  sseg.println(i);
  sseg.writeDisplay();
}
void SevenSeg::printHex(int i) {
  sseg.print(i, HEX);
  sseg.writeDisplay();
}

void SevenSeg::print(float f) {
  sseg.println(f);
  sseg.writeDisplay();
}
void SevenSeg::clear() {
  sseg.clear();
  sseg.writeDisplay();
}

void SevenSeg::showColon(bool col) {
  sseg.drawColon(col);
  sseg.writeDisplay();
}
