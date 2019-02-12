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
    
}

void BTN_Bzzr::This_Bzzr_State(int Bzzr_State){
    _Bzzr_State = Bzzr_State;
    digitalWrite(_pin, _Bzzr_State);
}

void BTN_Bzzr::Change_Bzzr_State(){
    int current_Bzzr_State = digitalRead(_pin);
    if(current_Bzzr_State == 1 ){
        digitalWrite (_pin,LOW);
    }else{
        digitalWrite(_pin, HIGH);
    }
}

void BTN_Bzzr::Beep(unsigned long beep_interval) {
    _beep_interval = beep_interval;
    digitalWrite(_pin, HIGH);
    delay(_beep_interval);
    digitalWrite(_pin, LOW);
}