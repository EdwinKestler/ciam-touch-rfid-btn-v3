/*
  TimeESP.cpp - Minimal time library for ESP8266
  Based on Time library by Michael Margolis (2009-2014), LGPL 2.1+
  Stripped to only functions used by this project.
*/

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include "TimeLibEsp.h"

static tmElements_t tm;
static time_t cacheTime;
static uint32_t syncInterval = 300;

static void refreshCache(time_t t) {
  if (t != cacheTime) {
    breakTime(t, tm);
    cacheTime = t;
  }
}

int hour()            { return hour(now()); }
int hour(time_t t)    { refreshCache(t); return tm.Hour; }
int minute()          { return minute(now()); }
int minute(time_t t)  { refreshCache(t); return tm.Minute; }
int second()          { return second(now()); }
int second(time_t t)  { refreshCache(t); return tm.Second; }
int day()             { return day(now()); }
int day(time_t t)     { refreshCache(t); return tm.Day; }
int month()           { return month(now()); }
int month(time_t t)   { refreshCache(t); return tm.Month; }
int year()            { return year(now()); }
int year(time_t t)    { refreshCache(t); return tmYearToCalendar(tm.Year); }

/*============================================================================*/

#define LEAP_YEAR(Y) ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )

static const uint8_t monthDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};

void breakTime(time_t timeInput, tmElements_t &tm) {
  uint8_t year;
  uint8_t month, monthLength;
  uint32_t time;
  unsigned long days;

  time = (uint32_t)timeInput;
  tm.Second = time % 60;
  time /= 60;
  tm.Minute = time % 60;
  time /= 60;
  tm.Hour = time % 24;
  time /= 24;
  tm.Wday = ((time + 4) % 7) + 1;

  year = 0;
  days = 0;
  while ((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time) {
    year++;
  }
  tm.Year = year;

  days -= LEAP_YEAR(year) ? 366 : 365;
  time -= days;

  for (month = 0; month < 12; month++) {
    if (month == 1) {
      monthLength = LEAP_YEAR(year) ? 29 : 28;
    } else {
      monthLength = monthDays[month];
    }
    if (time >= monthLength) {
      time -= monthLength;
    } else {
      break;
    }
  }
  tm.Month = month + 1;
  tm.Day = time + 1;
}

time_t makeTime(tmElements_t &tm) {
  int i;
  uint32_t seconds;

  seconds = tm.Year * (SECS_PER_DAY * 365);
  for (i = 0; i < tm.Year; i++) {
    if (LEAP_YEAR(i)) {
      seconds += SECS_PER_DAY;
    }
  }

  for (i = 1; i < tm.Month; i++) {
    if ((i == 2) && LEAP_YEAR(tm.Year)) {
      seconds += SECS_PER_DAY * 29;
    } else {
      seconds += SECS_PER_DAY * monthDays[i-1];
    }
  }
  seconds += (tm.Day-1) * SECS_PER_DAY;
  seconds += tm.Hour * SECS_PER_HOUR;
  seconds += tm.Minute * SECS_PER_MIN;
  seconds += tm.Second;
  return (time_t)seconds;
}

/*============================================================================*/

static uint32_t sysTime = 0;
static uint32_t prevMillis = 0;
static uint32_t nextSyncTime = 0;
static timeStatus_t Status = timeNotSet;

static getExternalTime getTimePtr;

time_t now() {
  while (millis() - prevMillis >= 1000) {
    sysTime++;
    prevMillis += 1000;
  }
  if (nextSyncTime <= sysTime) {
    if (getTimePtr != 0) {
      time_t t = getTimePtr();
      if (t != 0) {
        setTime(t);
      } else {
        nextSyncTime = sysTime + syncInterval;
        Status = (Status == timeNotSet) ? timeNotSet : timeNeedsSync;
      }
    }
  }
  return (time_t)sysTime;
}

void setTime(time_t t) {
  sysTime = (uint32_t)t;
  nextSyncTime = (uint32_t)t + syncInterval;
  Status = timeSet;
  prevMillis = millis();
}

void setTime(int hr, int min, int sec, int dy, int mnth, int yr) {
  if (yr > 99)
    yr = yr - 1970;
  else
    yr += 30;
  tm.Year = yr;
  tm.Month = mnth;
  tm.Day = dy;
  tm.Hour = hr;
  tm.Minute = min;
  tm.Second = sec;
  setTime(makeTime(tm));
}

timeStatus_t timeStatus() {
  now();
  return Status;
}

void setSyncProvider(getExternalTime getTimeFunction) {
  getTimePtr = getTimeFunction;
  nextSyncTime = sysTime;
  now();
}
