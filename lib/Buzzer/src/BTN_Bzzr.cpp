/*
  BTN_Bzzr.cpp - Library for audible alarma signal via piezo buzzer on device.
  Created by Edwin Kestler, JAN 29, 2019.
  Released into the public domain.
*/
#include "Arduino.h"
#include "BTN_Bzzr.h"

BTN_Bzzr::BTN_Bzzr(int pin) {
    pinMode(pin, OUTPUT);
    _pin = pin;
    _beeping = false;
    _beep_start = 0;
}

void BTN_Bzzr::Beep(unsigned long beep_interval) {
    _beep_interval = beep_interval;
    digitalWrite(_pin, HIGH);
    delay(_beep_interval);
    digitalWrite(_pin, LOW);
}

void BTN_Bzzr::BeepNonBlocking(unsigned long beep_interval) {
    _beep_interval = beep_interval;
    _beep_start = millis();
    _beeping = true;
    digitalWrite(_pin, HIGH);
}

void BTN_Bzzr::update() {
    if (_beeping && (millis() - _beep_start >= _beep_interval)) {
        digitalWrite(_pin, LOW);
        _beeping = false;
    }
}

bool BTN_Bzzr::isBeeping() {
    return _beeping;
}
