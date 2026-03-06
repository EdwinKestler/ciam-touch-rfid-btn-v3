/*
  pins.h - Centralized pin definitions for the CIAM Touch RFID Button platform.
  All hardware pin assignments in one place for easy porting and review.
*/
#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

// Touch button (capacitive sensor)
#define PIN_TOUCH       D0

// RFID reader (SoftwareSerial)
#define PIN_RFID_TX     D2
#define PIN_RFID_RX     D3

// Piezo buzzer
#define PIN_BUZZER      D5

// RGB LED (active-high individual channels)
#define PIN_LED_BLUE    D6
#define PIN_LED_GREEN   D7
#define PIN_LED_RED     D8

// Purple LED mix uses D4 (also ESP8266 built-in LED)
#define PIN_LED_PURPLE  D4

#endif // PINS_H
