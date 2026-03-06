/*
  ntp_sync.h - NTP time synchronization module.
  Owns the NTPClient instance, ISO8601 timestamp buffer, and sync logic.
*/
#ifndef NTP_SYNC_H
#define NTP_SYNC_H

#include <Arduino.h>

// Current ISO8601 timestamp (updated by ntp_check_time)
extern char ISO8601[21];

// True after a successful NTP response
extern boolean NTP_response;

// Initialize NTP client (call once in setup after config is loaded)
void ntp_init(const char* server, int updateInterval);

// Startup sync: retries up to 5 times (call once in setup after WiFi connected)
void ntp_sync_startup();

// Runtime resync: retries up to 5 times (called from STATE_UPDATE_TIME)
void ntp_resync();

// Update ISO8601 buffer from current time (call before publishing)
void ntp_check_time();

#endif // NTP_SYNC_H
