#include "log.h"

extern unsigned long start_interval_ms;

void logTime()
{
  struct tm* ptm;
  time_t now = time(NULL);

  ptm = gmtime(&now);

  Serial.print(ptm->tm_year + UNIX_EPOCH_START_YEAR);
  Serial.print("/");
  Serial.print(ptm->tm_mon + 1);
  Serial.print("/");
  Serial.print(ptm->tm_mday);
  Serial.print(" ");

  if (ptm->tm_hour < 10)
  {
    Serial.print(0);
  }

  Serial.print(ptm->tm_hour);
  Serial.print(":");

  if (ptm->tm_min < 10)
  {
    Serial.print(0);
  }

  Serial.print(ptm->tm_min);
  Serial.print(":");

  if (ptm->tm_sec < 10)
  {
    Serial.print(0);
  }

  Serial.println(ptm->tm_sec);
}

void logMsg(char * msg) 
{
  time_t now = time(NULL);
  Serial.print(now);Serial.print(" -- "); Serial.println(msg); 
}
void logErr(char * msg) 
{
  time_t now = time(NULL);
  Serial.print(now);Serial.print(" --ERR "); Serial.println(msg); 
}
void logMsg(char * msg, int num) 
{
  time_t now = time(NULL);
  Serial.print(now);Serial.print(" -- "); Serial.print(msg);
  Serial.println(num); 
}

void logMsgStr(char * msg, char *param) 
{
  time_t now = time(NULL);
  Serial.print(now);Serial.print(" -- "); Serial.print(msg);
  Serial.println(param); 
}

void logErr(char * msg, int num) 
{
  time_t now = time(NULL);
  Serial.print(now);Serial.print(" --Err "); Serial.print(msg);
  Serial.println(num); 
}
