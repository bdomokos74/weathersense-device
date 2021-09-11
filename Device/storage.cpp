#include "storage.h"
const char *measTemplate = "{\"Temperature\":%.2f,\"Pressure\":%.2f,\"Humidity\":%.2f}";

#define RTC_BUF_SIZE 3072
RTC_DATA_ATTR unsigned long startTimestamp;

RTC_DATA_ATTR char dataBuf[RTC_BUF_SIZE];
RTC_DATA_ATTR char *bufPoi = dataBuf;
RTC_DATA_ATTR int numStoredMeasurements = 0;
RTC_DATA_ATTR int msgId = 0;

Storage::Storage(BMESensor *bme, DallasSensor *dallas, State *state) {
    bmeSensor = bme;
    dallasSensor = dallas;
    deviceState = state;
}

int Storage::storeMeasurement() 
{
  int remainingLen = RTC_BUF_SIZE-(int)(bufPoi-dataBuf);
  
  boolean doSleep = deviceState->getDoSleep();
  int sleepTimeSec = deviceState->getSleepTimeSec();
  float temp;
  float temp2;
  float pres;
  float hum;
  int bat = analogRead(A13);
  float battery = (bat*2)/4095.0F*3.3F;
  
  int writtenChars = 0;
  if(numStoredMeasurements==0) startTimestamp = millis();
  unsigned long curr = millis();
  if(doSleep && numStoredMeasurements>0) {
    curr += numStoredMeasurements*sleepTimeSec*1000;
  }
  
  if(bmeSensor->isConnected()) {
    temp = bmeSensor->readTemp();
    pres = bmeSensor->readPressure();
    hum = bmeSensor->readHumidity();
  } 

  if(dallasSensor->isConnected()) {
      temp2 = dallasSensor->readTemp();
  }

  az_span span = az_span_create((uint8_t*)bufPoi, (int32_t)remainingLen);

  char tmp[20];
  az_span next = az_span_copy(span, AZ_SPAN_LITERAL_FROM_STR("{"));
  az_span tmpSpan;

  next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR("\"id\":"));
  snprintf(tmp, 10, "%d", msgId++);
  tmpSpan = az_span_create_from_str(tmp);
  next = az_span_copy(next, tmpSpan);

  next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR(",\"ts\":"));
  time_t now = time(NULL);
  snprintf(tmp, 10, "%lu", (unsigned long)now);
  tmpSpan = az_span_create_from_str(tmp);
  next = az_span_copy(next, tmpSpan);

  
  if(bmeSensor->isConnected()&&dallasSensor->isConnected()) {
    next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR(",\"t1\":"));
    snprintf(tmp, 10, "%.0f", temp);
    tmpSpan = az_span_create_from_str(tmp);
    next = az_span_copy(next, tmpSpan);
    next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR(",\"t2\":"));
    snprintf(tmp, 10, "%.0f", temp2);
    tmpSpan = az_span_create_from_str(tmp);
    next = az_span_copy(next, tmpSpan);
    //writtenChars = snprintf(bufPoi, remainingLen, bothTemplate, msgId++, temp, pres, hum, battery,currTime,temp2);
  } else if( bmeSensor->isConnected()) {
    next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR(",\"t1\":"));
    snprintf(tmp, 10, "%.2f", temp);
    tmpSpan = az_span_create_from_str(tmp);
    next = az_span_copy(next, tmpSpan);
    //writtenChars = snprintf(bufPoi, remainingLen, bmeMessageTemplate, msgId++, temp, pres, hum, battery,currTime);
  } else if (dallasSensor->isConnected()) {
    next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR(",\"t1\":"));
    snprintf(tmp, 10, "%.2f", temp2);
    tmpSpan = az_span_create_from_str(tmp);
    next = az_span_copy(next, tmpSpan);
      //writtenChars = snprintf(bufPoi, remainingLen, dallasMessageTemplate, msgId++, temp2, battery, currTime);
  } 
  if(bmeSensor->isConnected()) {
    next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR(",\"p\":"));
    snprintf(tmp, 10, "%.2f", pres);
    tmpSpan = az_span_create_from_str(tmp);
    next = az_span_copy(next, tmpSpan);
    next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR(",\"h\":"));
    snprintf(tmp, 10, "%.2f", hum);
    tmpSpan = az_span_create_from_str(tmp);
    next = az_span_copy(next, tmpSpan);
  }
  next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR(",\"bat\":"));
  snprintf(tmp, 10, "%.2f", battery);
  tmpSpan = az_span_create_from_str(tmp);
  next = az_span_copy(next, tmpSpan);

  next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR("}\n"));
  az_span_ptr(next)[0] = '\0';

  //logMsg((char*)az_span_ptr(span));
  writtenChars = (int)(az_span_ptr(next)-az_span_ptr(span));
  bufPoi += writtenChars;
  if(writtenChars > 0)
    numStoredMeasurements++;
  return writtenChars;
}

int Storage::getMeasurementString(char* buf, int size) {
  int writtenChars = 0;
  float temp;
  float pres;
  float hum;
  if(bmeSensor->isConnected()) {
    temp = bmeSensor->readTemp();
    pres = bmeSensor->readPressure();
    hum = bmeSensor->readHumidity();
    writtenChars = snprintf(buf, size, measTemplate, temp, pres, hum);
  }
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
