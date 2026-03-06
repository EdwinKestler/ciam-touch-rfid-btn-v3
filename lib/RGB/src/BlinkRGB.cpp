/*
  BlinkRGB.cpp - Library for RGB visual signal.
  Created by Edwin Kestler, May 29, 2018.
  Released into the public domain.
*/
#include "Arduino.h"
#include "BlinkRGB.h"

BlinkRGB::BlinkRGB(int pin) {
    pinMode(pin, OUTPUT);
    _pin = pin;
    _flashing = false;
    _flash_start = 0;
}

void BlinkRGB::This_RGB_State(int RGB_State){
    _RGB_State = RGB_State;
    digitalWrite(_pin, _RGB_State);
}

/*
void BlinkRGB::Change_RGB_State(){
    int current_RGB_State = digitalRead(_pin);
    if(current_RGB_State == 1 ){
        digitalWrite (_pin,LOW);
    }else{
        digitalWrite(_pin, HIGH);
    }
}
*/

void BlinkRGB::SetPWM(int value) {
    // Map 0-255 to ESP8266 analogWrite range 0-1023
    int pwm = constrain(value, 0, 255) * 4;
    analogWrite(_pin, pwm);
}

void BlinkRGB::Flash(unsigned long flash_interval) {
    _flash_interval = flash_interval;
    digitalWrite(_pin, HIGH);
    delay(_flash_interval);
    digitalWrite(_pin, LOW);
}

void BlinkRGB::FlashNonBlocking(unsigned long flash_interval) {
    _flash_interval = flash_interval;
    _flash_start = millis();
    _flashing = true;
    digitalWrite(_pin, HIGH);
}

void BlinkRGB::update() {
    if (_flashing && (millis() - _flash_start >= _flash_interval)) {
        digitalWrite(_pin, LOW);
        _flashing = false;
    }
}

BlinkColor::BlinkColor(int pin0, int pin1,int pin2) {
    pinMode(pin0, OUTPUT);
    pinMode(pin1, OUTPUT);
    pinMode(pin2, OUTPUT);
    _pin0 = pin0;
    _pin1 = pin1;
    _pin2 = pin2;
    _flashing = false;
    _flash_start = 0;
}

void BlinkColor::COn() {
    digitalWrite(_pin0, HIGH);
    digitalWrite(_pin1, HIGH);
    digitalWrite(_pin2, HIGH);
}

void BlinkColor::COff() {
    digitalWrite(_pin0, LOW);
    digitalWrite(_pin1, LOW);
    digitalWrite(_pin2, LOW);
}


void BlinkColor::SetRGB(int r, int g, int b) {
    // Map 0-255 to ESP8266 analogWrite range 0-1023
    // pin0=Blue(D6), pin1=Green(D7), pin2=Red(D8) per main.cpp wiring
    analogWrite(_pin2, constrain(r, 0, 255) * 4);
    analogWrite(_pin1, constrain(g, 0, 255) * 4);
    analogWrite(_pin0, constrain(b, 0, 255) * 4);
}

void BlinkColor::CFlash(unsigned long Cflash_interval) {
    _Cflash_interval = Cflash_interval;
    digitalWrite(_pin0, HIGH);
    digitalWrite(_pin1, HIGH);
    digitalWrite(_pin2, HIGH);
    delay(_Cflash_interval);
    digitalWrite(_pin0, LOW);
    digitalWrite(_pin1, LOW);
    digitalWrite(_pin2, LOW);
}

void BlinkColor::CFlashNonBlocking(unsigned long Cflash_interval) {
    _Cflash_interval = Cflash_interval;
    _flash_start = millis();
    _flashing = true;
    digitalWrite(_pin0, HIGH);
    digitalWrite(_pin1, HIGH);
    digitalWrite(_pin2, HIGH);
}

void BlinkColor::update() {
    if (_flashing && (millis() - _flash_start >= _Cflash_interval)) {
        digitalWrite(_pin0, LOW);
        digitalWrite(_pin1, LOW);
        digitalWrite(_pin2, LOW);
        _flashing = false;
    }
}