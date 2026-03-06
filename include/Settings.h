/*
  Settings.h - Compile-time constants for the CIAM Touch RFID Button platform.
  Created by Edwin Kestler, Jan 29, 2019.
  Released into the public domain.

  NOTE: Runtime-configurable variables (intervals, thresholds) are declared
  in this header but defined in Settings.cpp so they exist once in the binary.
*/
#ifndef SETTINGS_H
#define SETTINGS_H

// OTA credentials
extern const char* ssid_OTA;
extern const char* password_OTA;

// MQTT organization / device type (used to build client ID)
#define ORG "FLATBOX"
#define DEVICE_TYPE "AC_WIFI_RFID_BTN"

// Timezone offset (hours from UTC)
extern const int timeZone;

// Firmware and hardware version strings
extern const char FirmwareVersion[];
extern const char HardwareVersion[];

// MQTT topic names
extern char publishTopic[];
extern char responseTopic[];
extern char manageTopic[];
extern char updateTopic[];
extern char rebootTopic[];
extern char rgbTopic[];

// Runtime-configurable variables (can be changed via MQTT update topic)
extern unsigned long Universal_1_sec_Interval;
extern unsigned long Btn_conf_Mode_Interval;
extern int heartbeat_minutes;
extern int rssi_low_threshold;
extern int fail_threshold;

#endif // SETTINGS_H
