/*
  ntp_sync.cpp - NTP time synchronization module.
*/
#include "ntp_sync.h"
#include <NTP_Client.h>
#include <TimeLibEsp.h>
#include <Settings.h>

// --- Globals ---
char ISO8601[21] = "";
boolean NTP_response = false;

static NTPClient* pTimeClient = nullptr;

// --- NTP request (used as setSyncProvider callback) ---
static time_t NTP_ready() {
  Serial.println(F("Transmit NTP Request"));
  pTimeClient->update();
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    Serial.print(".");
    delay(100);
    unsigned long EpochTime = pTimeClient->getRawTime();
    if (EpochTime >= 3610) {
      Serial.println();
      Serial.println(F("Receive NTP Response"));
      NTP_response = true;
      return EpochTime + (timeZone * SECS_PER_HOUR);
    }
  }
  Serial.println();
  Serial.println(F("No NTP Response :-("));
  NTP_response = false;
  return 0;
}

void ntp_init(const char* server, int updateInterval) {
  pTimeClient = new NTPClient(server, 0, updateInterval);
}

void ntp_sync_startup() {
  pTimeClient->begin();

  for (int ntp_retry = 0; ntp_retry < 5 && !NTP_response; ntp_retry++) {
    Serial.print(F("NTP sync attempt #"));
    Serial.println(ntp_retry + 1);
    setSyncProvider(NTP_ready);
    delay(5 * Universal_1_sec_Interval);
  }
  if (!NTP_response) {
    Serial.println(F("NTP sync failed after 5 attempts, continuing without time"));
  }
}

void ntp_resync() {
  Serial.println(F("NTP_CLIENT"));
  pTimeClient->update();
  int ntpRetry = 0;
  while (!NTP_response && ntpRetry < 5) {
    setSyncProvider(NTP_ready);
    delay(Universal_1_sec_Interval);
    ntpRetry++;
  }
  if (!NTP_response) {
    Serial.println(F("NTP sync failed after 5 attempts, continuing..."));
  }
  NTP_response = false;
}

void ntp_check_time() {
  static time_t prevDisplay = 0;
  if (timeStatus() != timeNotSet) {
    time_t t = now();
    if (t != prevDisplay) {
      prevDisplay = t;
      snprintf(ISO8601, sizeof(ISO8601), "%04d-%02d-%02dT%02d:%02d:%02d", year(), month(), day(), hour(), minute(), second());
    }
  } else {
    Serial.println(F("Time not Sync, Syncronizing time"));
    NTP_ready();
  }
}
