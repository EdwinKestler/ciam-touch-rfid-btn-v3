/*
  Settings.cpp - Definitions for compile-time constants and runtime-configurable variables.
*/
#include <Settings.h>

// OTA credentials
const char* ssid_OTA = "RFID_OTA";
const char* password_OTA = "FLATB0X_OTA";

// Timezone
const int timeZone = -6;  // Central Time (USA)

// Version strings
const char FirmwareVersion[] = "V6.00";
const char HardwareVersion[] = "V3.00";

// MQTT topic names
char publishTopic[]  = "iot-2/evt/status/fmt/json";
char responseTopic[] = "iotdm-1/response/";
char manageTopic[]   = "iotdevice-1/mgmt/manage";
char updateTopic[]   = "iotdm-1/device/update";
char rebootTopic[]   = "iotdm-1/mgmt/initiate/device/reboot";
char rgbTopic[]      = "iotdm-1/device/ctrl";

// Runtime-configurable variables (defaults, changeable via MQTT)
unsigned long Universal_1_sec_Interval = 1000UL;
unsigned long Btn_conf_Mode_Interval   = 2000UL;
int heartbeat_minutes   = 30;
int rssi_low_threshold  = -75;
int fail_threshold      = 150;
