/*
  ota.cpp - OTA update mode setup.
*/
#include "ota.h"
#include "feedback.h"
#include <Settings.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>

#define STATE_RDY_TO_UPDATE_OTA 7

void bootToOTA(int& fsm_state) {
  Serial.println(F("Starting OTA"));
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid_OTA, password_OTA);
  ArduinoOTA.setPassword(password_OTA);
  ArduinoOTA.onStart([]() {
    Serial.println(F("OTA: update starting..."));
    Azul.This_RGB_State(HIGH);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\nOTA: update complete, rebooting"));
    Azul.This_RGB_State(LOW);
    alarm_buzzer.Beep(tono_corto);
    alarm_buzzer.Beep(tono_corto);
    alarm_buzzer.Beep(tono_corto);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
    Rojo.Flash(flash_medio);
    alarm_buzzer.Beep(tono_medio);
    alarm_buzzer.Beep(tono_medio);
  });
  ArduinoOTA.begin();
  Serial.println(F("Ready"));
  Azul.Flash(flash_medio);
  fsm_state = STATE_RDY_TO_UPDATE_OTA;
  alarm_buzzer.Beep(tono_medio);
  alarm_buzzer.Beep(tono_corto);
  alarm_buzzer.Beep(tono_medio);
}
