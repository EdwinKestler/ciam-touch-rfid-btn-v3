/*
  TimeLibEsp.h - Minimal time library for ESP8266
  Based on TimeLib by Michael Margolis (2009-2014)
  Stripped to only functions used by this project.
*/

#ifndef _TimeEsp_h
#ifdef __cplusplus
#define _TimeEsp_h

#include <inttypes.h>
#ifndef __AVR__
#include <sys/types.h>
#endif

#if !defined(__time_t_defined)
typedef unsigned long time_t;
#endif

extern "C++" {

typedef enum { timeNotSet, timeNeedsSync, timeSet } timeStatus_t;

typedef struct {
  uint8_t Second;
  uint8_t Minute;
  uint8_t Hour;
  uint8_t Wday;   // day of week, sunday is day 1
  uint8_t Day;
  uint8_t Month;
  uint8_t Year;   // offset from 1970
} tmElements_t;

#define tmYearToCalendar(Y) ((Y) + 1970)
#define CalendarYrToTm(Y)   ((Y) - 1970)

typedef time_t(*getExternalTime)();

#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24UL)

int     hour();
int     hour(time_t t);
int     minute();
int     minute(time_t t);
int     second();
int     second(time_t t);
int     day();
int     day(time_t t);
int     month();
int     month(time_t t);
int     year();
int     year(time_t t);

time_t  now();
void    setTime(time_t t);
void    setTime(int hr, int min, int sec, int day, int month, int yr);

timeStatus_t timeStatus();
void    setSyncProvider(getExternalTime getTimeFunction);

void    breakTime(time_t time, tmElements_t &tm);
time_t  makeTime(tmElements_t &tm);

} // extern "C++"
#endif // __cplusplus
#endif // _TimeEsp_h
