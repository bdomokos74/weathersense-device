#include "storage.h"

const char *bmeMessageTemplate = "{\"messageId\":%d,\"Temperature\":%.2f,\"Pressure\":%.2f,\"Humidity\":%.2f,\"bat\":%.2f,\"offset\":%d}\n";
const char *dallasMessageTemplate = "{\"messageId\":%d,\"Temperature\":%.2f,\"bat\":%.2f,\"offset\":%d}\n"; 
const char *bothTemplate = "{\"Id\":%d,\"t1\":%.2f,\"p\":%.2f,\"h\":%.2f,\"bat\":%.2f,\"offset\":%d,\"t2\":%.2f}\n";

#define RTC_BUF_SIZE 3072
RTC_DATA_ATTR unsigned long startTimestamp;

RTC_DATA_ATTR char dataBuf[RTC_BUF_SIZE];
RTC_DATA_ATTR char *bufPoi = dataBuf;
RTC_DATA_ATTR int numStoredMeasurements = 0;
RTC_DATA_ATTR int msgId = 0;

Storage::Storage(BMESensor *bme, DallasSensor *dallas) {
    bmeSensor = bme;
    dallasSensor = dallas;
}

int Storage::storeMeasurement(boolean doSleep, int sleepTimeSec) {
    float temp;
    float temp2;
    float pres;
    float hum;
    int bat = analogRead(A13);
    float battery = (bat*2)/4095.0F*3.3F;
    int remainingLen = RTC_BUF_SIZE-(int)(bufPoi-dataBuf);
    int writtenChars = 0;
    if(numStoredMeasurements==0) startTimestamp = millis();
    unsigned long curr = millis();
    if(doSleep && numStoredMeasurements>0) {
      curr += numStoredMeasurements*sleepTimeSec*1000;
    }
    int currTime =  curr-startTimestamp;
    Serial.print("startTimestamp/curr/currtime = "); 
    Serial.print(startTimestamp); Serial.print("/"); Serial.print(curr);
    Serial.print("/"); Serial.println(currTime);
    
    if(bmeSensor->isConnected()) {
      temp = bmeSensor->readTemp();
      pres = bmeSensor->readPressure();
      hum = bmeSensor->readHumidity();
    } 
    Serial.println("bme done");

    if(dallasSensor->isConnected()) {
        temp2 = dallasSensor->readTemp();
    }

    if(bmeSensor->isConnected()&&dallasSensor->isConnected()) {
      writtenChars = snprintf(bufPoi, remainingLen, bothTemplate, msgId++, temp, pres, hum, battery,currTime,temp2);
    } else if( bmeSensor->isConnected()) {
      writtenChars = snprintf(bufPoi, remainingLen, bmeMessageTemplate, msgId++, temp, pres, hum, battery,currTime);
    } else if (dallasSensor->isConnected()) {
       writtenChars = snprintf(bufPoi, remainingLen, dallasMessageTemplate, msgId++, temp2, battery, currTime);
    } 

   bufPoi += writtenChars;
   if(writtenChars > 0)
    numStoredMeasurements++;
  return writtenChars;
}

void Storage::printStatus() {
    Serial.print("Measurement stored: numStored/datalen: ");
      Serial.print(numStoredMeasurements);Serial.print("/");
      Serial.println((int)(bufPoi-dataBuf));
}

void Storage::reset() {
    bufPoi = dataBuf;
    numStoredMeasurements = 0;
}

int Storage::getNumStoredMeasurements(){
    return numStoredMeasurements;
}

char *Storage::getDataBuf() {
    return dataBuf;
}