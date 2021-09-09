#ifndef __D_LOG_H
#define __D_LOG_H

#include "Arduino.h"

#define UNIX_EPOCH_START_YEAR 1900

void logErr(char * msg);
void logErr(char * msg, int num);
void logMsg(char * msg);
void logMsg(char * msg, int num);
void logMsgStr(char * msg, char *param);
void logTime();
#endif