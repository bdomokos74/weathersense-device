#include "storage.h"
const char *measTemplate = "Temperature:%.2f, Pressure:%.2f, Humidity:%.2f";

// TODO make sure when the buffer is full, no more events are stored, and the results are sent out
#define RTC_BUF_SIZE 3072

RTC_DATA_ATTR unsigned long startTimestamp;

RTC_DATA_ATTR char dataBuf[RTC_BUF_SIZE];
RTC_DATA_ATTR char *bufPoi = dataBuf;
RTC_DATA_ATTR int numStoredMeasurements = 0;
RTC_DATA_ATTR int msgId = 0;
RTC_DATA_ATTR bool bufferFull = false;

Storage::Storage(BMESensor *bme, DallasSensor *dallas, GMSensor *gm, State *state) {
    bmeSensor = bme;
    dallasSensor = dallas;
    deviceState = state;
    gmSensor = gm;
}

int Storage::getRemainingLen() 
{
  return (int)(RTC_BUF_SIZE-(int)(bufPoi-dataBuf));
}

byte gmBuf[8];
int Storage::storeMeasurement() 
{
  int remainingLen = getRemainingLen();
  boolean hasMeasurement = false;
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

  if(bmeSensor->isConnected()) {
    temp = bmeSensor->readTemp();
    pres = bmeSensor->readPressure();
    hum = bmeSensor->readHumidity();
  } 

  if(dallasSensor->isConnected()) {
      temp2 = dallasSensor->readTemp();
  }

  unsigned long cpm;
  bool hasCpm = false;

  if(gmSensor->isConnected()) {
    hasCpm = gmSensor->read(gmBuf);
    memcpy(&cpm, gmBuf+sizeof(long), sizeof(long));
  }

  char measBuf[200];
  az_span span = az_span_create((uint8_t*)measBuf, sizeof(measBuf));
  char tmp[20];
  az_span next = az_span_copy(span, AZ_SPAN_LITERAL_FROM_STR("{"));
  az_span tmpSpan;

  next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR("\"id\":"));
  snprintf(tmp, sizeof(tmp), "%d", msgId++);
  tmpSpan = az_span_create_from_str(tmp);
  next = az_span_copy(next, tmpSpan);

  next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR(",\"ts\":"));
  time_t now = time(NULL);
  snprintf(tmp, sizeof(tmp), "%lu", (unsigned long)now);
  tmpSpan = az_span_create_from_str(tmp);
  next = az_span_copy(next, tmpSpan);

  
  if(bmeSensor->isConnected()&&dallasSensor->isConnected()) {
    hasMeasurement = true;
    next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR(",\"t1\":"));
    snprintf(tmp, sizeof(tmp), "%.2f", temp);
    tmpSpan = az_span_create_from_str(tmp);
    next = az_span_copy(next, tmpSpan);
    next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR(",\"t2\":"));
    snprintf(tmp, sizeof(tmp), "%.2f", temp2);
    tmpSpan = az_span_create_from_str(tmp);
    next = az_span_copy(next, tmpSpan);
    //writtenChars = snprintf(bufPoi, remainingLen, bothTemplate, msgId++, temp, pres, hum, battery,currTime,temp2);
  } else if( bmeSensor->isConnected()) {
    hasMeasurement = true;
    next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR(",\"t1\":"));
    snprintf(tmp, sizeof(tmp), "%.2f", temp);
    tmpSpan = az_span_create_from_str(tmp);
    next = az_span_copy(next, tmpSpan);
    //writtenChars = snprintf(bufPoi, remainingLen, bmeMessageTemplate, msgId++, temp, pres, hum, battery,currTime);
  } else if (dallasSensor->isConnected()) {
    hasMeasurement = true;
    next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR(",\"t1\":"));
    snprintf(tmp, sizeof(tmp), "%.2f", temp2);
    tmpSpan = az_span_create_from_str(tmp);
    next = az_span_copy(next, tmpSpan);
      //writtenChars = snprintf(bufPoi, remainingLen, dallasMessageTemplate, msgId++, temp2, battery, currTime);
  } 
  if(bmeSensor->isConnected()) {
    hasMeasurement = true;
    next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR(",\"p\":"));
    snprintf(tmp, sizeof(tmp), "%.2f", pres);
    tmpSpan = az_span_create_from_str(tmp);
    next = az_span_copy(next, tmpSpan);
    next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR(",\"h\":"));
    snprintf(tmp, sizeof(tmp), "%.2f", hum);
    tmpSpan = az_span_create_from_str(tmp);
    next = az_span_copy(next, tmpSpan);
  }
 
  if(gmSensor->isConnected() && hasCpm) {
    hasMeasurement = true;
    next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR(",\"cpm\":"));
    snprintf(tmp, sizeof(tmp), "%d", cpm);
    tmpSpan = az_span_create_from_str(tmp);
    next = az_span_copy(next, tmpSpan);
  }
  if(!hasMeasurement) {
    Serial.println("skipping, as no meas");
    return 0;
  }
  next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR(",\"bat\":"));
  snprintf(tmp, sizeof(tmp), "%.2f", battery);
  tmpSpan = az_span_create_from_str(tmp);
  next = az_span_copy(next, tmpSpan);

  next = az_span_copy(next, AZ_SPAN_LITERAL_FROM_STR("}\n"));
  az_span_ptr(next)[0] = '\0';
  
  writtenChars = (int)(az_span_ptr(next)-az_span_ptr(span));
  logMsg("meas size:",writtenChars);
  logMsg("remaining len:",remainingLen);
  if(remainingLen>writtenChars) {
    az_span rtcSpan = az_span_create((uint8_t*)bufPoi, (int32_t)remainingLen);
    az_span measSpan = az_span_create_from_str((char*)az_span_ptr(span));
    az_span_copy(rtcSpan, measSpan);
    bufPoi += writtenChars;
    bufPoi[0] = '\0';
    if(writtenChars > 0)
      numStoredMeasurements++;
  } else {
    // Can't add the measurement, need to send first
    bufferFull = true;
  }
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
    bufferFull = false;
}

int Storage::getNumStoredMeasurements(){
    return numStoredMeasurements;
}

char *Storage::getDataBuf() {
    return dataBuf;
}

bool Storage::isBufferFull()
{
  return bufferFull;
}
