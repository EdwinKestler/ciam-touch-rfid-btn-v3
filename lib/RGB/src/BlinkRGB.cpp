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
}

void BlinkRGB::This_RGB_State(int RGB_State){
    _RGB_State = RGB_State;
    digitalWrite(_pin, _RGB_State);
}

void BlinkRGB::Change_RGB_State(){
    int current_RGB_State = digitalRead(_pin);
    if(current_RGB_State == 1 ){
        digitalWrite (_pin,LOW);
    }else{
        digitalWrite(_pin, HIGH);
    }
}

void BlinkRGB::Flash(unsigned long flash_interval) {
    _flash_interval = flash_interval;
    digitalWrite(_pin, HIGH);
    delay(_flash_interval);
    digitalWrite(_pin, LOW);
}

BlinkColor::BlinkColor(int pin0, int pin1,int pin2) {
    pinMode(pin0, OUTPUT);
    pinMode(pin1, OUTPUT);
    pinMode(pin2, OUTPUT);
    _pin0 = pin0;
    _pin1 = pin1;
    _pin2 = pin1;
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