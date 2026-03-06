/*
  config.h - Device configuration: struct, LittleFS read/save, XOR obfuscation, WiFiManager params.
*/
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

#define CONFIG_VERSION 3
#define XOR_KEY 0xA5

struct BtnConf {
    char  MQTT_Server[30];
    char  MQTT_Port[6]           = "1883";
    char  MQTT_User[24];
    char  MQTT_Password[24];
    char  NTPClient_SERVER[24]   = "time-a-g.nist.gov";
    char  NTPClient_interval[6];
    char  Device_ID[16]          = "CIAM";
    char  Location[48]           = "";
    char  Wifi_Fallback_SSID[33] = "";
    char  Wifi_Fallback_Pass[64] = "";
};

extern BtnConf btnconfig;
extern bool shouldSaveConfig;

// XOR + hex encode/decode (JSON-safe obfuscation)
void xorObfuscateHex(const char* input, char* output, size_t outLen);
void xorDeobfuscateHex(const char* hexInput, char* output, size_t outLen);

// LittleFS config operations
void readConfigFromLittleFS();
void saveConfigToLittleFS();

// WiFiManager helpers
void saveConfigCallback();
void setupWifiManagerParams(WiFiManager &wm,
    WiFiManagerParameter &srv, WiFiManagerParameter &port, WiFiManagerParameter &usr,
    WiFiManagerParameter &pwd, WiFiManagerParameter &ntp_srv, WiFiManagerParameter &ntp_int,
    WiFiManagerParameter &dev_id, WiFiManagerParameter &loc,
    WiFiManagerParameter &fb_ssid, WiFiManagerParameter &fb_pass);
void copyWifiManagerParams(
    WiFiManagerParameter &srv, WiFiManagerParameter &port, WiFiManagerParameter &usr,
    WiFiManagerParameter &pwd, WiFiManagerParameter &ntp_srv, WiFiManagerParameter &ntp_int,
    WiFiManagerParameter &dev_id, WiFiManagerParameter &loc,
    WiFiManagerParameter &fb_ssid, WiFiManagerParameter &fb_pass);

#endif // CONFIG_H
