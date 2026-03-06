/*
  wifi_setup.h - WiFiManager portal modes and fallback WiFi.
*/
#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include <Arduino.h>

// On-demand WiFi config portal (user holds button during first boot window)
void bootToOnDemandWifiManager();

// Auto WiFi connect with fallback to config portal
void bootToWifiManager();

// Try connecting to fallback WiFi SSID (returns true if connected)
bool tryFallbackWifi();

#endif // WIFI_SETUP_H
