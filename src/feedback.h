/*
  feedback.h - Unified audio/visual feedback layer.
  Abstracts LED and buzzer hardware so callers don't need to know pin details.
  Change hardware (e.g., WS2812) by editing only feedback.cpp.
*/
#ifndef FEEDBACK_H
#define FEEDBACK_H

#include <Arduino.h>
#include <BlinkRGB.h>
#include <BTN_Bzzr.h>
#include "pins.h"

// Configurable durations (remotely adjustable via MQTT update topic)
extern unsigned long tono_corto;
extern unsigned long tono_medio;
extern unsigned long tono_largo;
extern unsigned long flash_corto;
extern unsigned long flash_medio;
extern unsigned long flash_largo;

// Direct access to hardware objects (needed by OTA, RGB command handler, etc.)
extern BlinkRGB Azul;
extern BlinkRGB Verde;
extern BlinkRGB Rojo;
extern BlinkColor Blanco;
extern BlinkColor Purpura;
extern BTN_Bzzr alarm_buzzer;

// Semantic feedback functions — single place to change when hardware changes
void feedback_ok();         // short green flash + short beep
void feedback_error();      // short red flash + medium beep
void feedback_warning();    // medium red flash + medium beep
void feedback_alarm();      // long white flash + long beep (max 4x per cycle)
void feedback_publish_ok(); // short green flash + short beep (after successful MQTT publish)
void feedback_publish_fail(); // short red flash (after failed MQTT publish)
void feedback_mqtt_retry(); // short white flash + short beep (MQTT reconnect attempt)
void feedback_mqtt_fail();  // medium purple flash + medium beep (MQTT reconnect failed)
void feedback_card_read();  // short green flash + short beep (RFID card detected)
void feedback_button_press(); // short blue flash + short beep

// Call in loop() for non-blocking buzzer
void feedback_update();

#endif // FEEDBACK_H
