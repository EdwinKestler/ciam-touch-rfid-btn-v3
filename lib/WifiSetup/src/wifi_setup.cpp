/*
  wifi_setup.cpp - WiFiManager portal modes and fallback WiFi.
*/
#include "wifi_setup.h"
#include <config.h>
#include <feedback.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

void bootToOnDemandWifiManager() {
  WiFiManagerParameter custom_mqtt_server("server", "MQTT_Server", btnconfig.MQTT_Server, 64);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT_Port", btnconfig.MQTT_Port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "MQTT_User", btnconfig.MQTT_User, 64);
  WiFiManagerParameter custom_mqtt_pass("password", "MQTT_Password", btnconfig.MQTT_Password, 64);
  WiFiManagerParameter custom_NTPClient_SERVER("NTPServer", "NTPClient_SERVER", btnconfig.NTPClient_SERVER, 64);
  WiFiManagerParameter custom_NTPClient_interval("NTPInterval", "NTPClient_interval", btnconfig.NTPClient_interval, 6);
  WiFiManagerParameter custom_Device_ID("deviceid", "Device_ID", btnconfig.Device_ID, 16);
  WiFiManagerParameter custom_Location("location", "Location", btnconfig.Location, 48);
  WiFiManagerParameter custom_Fallback_SSID("fb_ssid", "Fallback_SSID", btnconfig.Wifi_Fallback_SSID, 33);
  WiFiManagerParameter custom_Fallback_Pass("fb_pass", "Fallback_Pass", btnconfig.Wifi_Fallback_Pass, 64);

  WiFiManager wifiManager;
  setupWifiManagerParams(wifiManager, custom_mqtt_server, custom_mqtt_port, custom_mqtt_user,
                         custom_mqtt_pass, custom_NTPClient_SERVER, custom_NTPClient_interval, custom_Device_ID,
                         custom_Location, custom_Fallback_SSID, custom_Fallback_Pass);

  Serial.println(F("Empezando Configuracion de WIFI Bajo Demanda"));
  Purpura.COn();
  alarm_buzzer.Beep(tono_medio);
  alarm_buzzer.Beep(tono_medio);
  if (!wifiManager.startConfigPortal("flatwifi")) {
    ESP.reset();
    delay(5000);
  }

  copyWifiManagerParams(custom_mqtt_server, custom_mqtt_port, custom_mqtt_user,
                        custom_mqtt_pass, custom_NTPClient_SERVER, custom_NTPClient_interval, custom_Device_ID,
                        custom_Location, custom_Fallback_SSID, custom_Fallback_Pass);
}

void bootToWifiManager() {
  WiFiManagerParameter custom_mqtt_server("server", "MQTT_Server", btnconfig.MQTT_Server, 64);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT_Port", btnconfig.MQTT_Port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "MQTT_User", btnconfig.MQTT_User, 64);
  WiFiManagerParameter custom_mqtt_pass("password", "MQTT_Password", btnconfig.MQTT_Password, 64);
  WiFiManagerParameter custom_NTPClient_SERVER("NTPServer", "NTPClient_SERVER", btnconfig.NTPClient_SERVER, 64);
  WiFiManagerParameter custom_NTPClient_interval("NTPInterval", "NTPClient_interval", btnconfig.NTPClient_interval, 6);
  WiFiManagerParameter custom_Device_ID("deviceid", "Device_ID", btnconfig.Device_ID, 16);
  WiFiManagerParameter custom_Location("location", "Location", btnconfig.Location, 48);
  WiFiManagerParameter custom_Fallback_SSID("fb_ssid", "Fallback_SSID", btnconfig.Wifi_Fallback_SSID, 33);
  WiFiManagerParameter custom_Fallback_Pass("fb_pass", "Fallback_Pass", btnconfig.Wifi_Fallback_Pass, 64);

  WiFiManager wifiManager;
  setupWifiManagerParams(wifiManager, custom_mqtt_server, custom_mqtt_port, custom_mqtt_user,
                         custom_mqtt_pass, custom_NTPClient_SERVER, custom_NTPClient_interval, custom_Device_ID,
                         custom_Location, custom_Fallback_SSID, custom_Fallback_Pass);

  Serial.println(F("Empezando Configuracion de WIFI en Automatico"));
  Purpura.COn();
  if (!wifiManager.autoConnect("flatwifi")) {
    alarm_buzzer.Beep(tono_corto);
    alarm_buzzer.Beep(tono_corto);
    Purpura.CFlash(flash_medio);
    if (!wifiManager.startConfigPortal("flatwifi")) {
      ESP.reset();
      delay(5000);
    }
  }

  copyWifiManagerParams(custom_mqtt_server, custom_mqtt_port, custom_mqtt_user,
                        custom_mqtt_pass, custom_NTPClient_SERVER, custom_NTPClient_interval, custom_Device_ID,
                        custom_Location, custom_Fallback_SSID, custom_Fallback_Pass);
}

bool tryFallbackWifi() {
  if (strlen(btnconfig.Wifi_Fallback_SSID) == 0) {
    return false;
  }
  Serial.print(F("Intentando red WiFi de respaldo: "));
  Serial.println(btnconfig.Wifi_Fallback_SSID);
  WiFi.begin(btnconfig.Wifi_Fallback_SSID, btnconfig.Wifi_Fallback_Pass);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("Conectado a red WiFi de respaldo"));
    return true;
  }
  Serial.println(F("Fallo conexion a red WiFi de respaldo"));
  return false;
}
