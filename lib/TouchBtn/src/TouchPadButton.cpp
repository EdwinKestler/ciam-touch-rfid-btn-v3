/*
  TouchPadButton.h - Library for Touch Pad as a Button signal.
  Created by Edwin Kestler, May 29, 2018.
  Released into the public domain.
*/

#include "Arduino.h"
#include "TouchPadButton.h"

TouchPadButton::TouchPadButton(int pin){
    pinMode(pin, INPUT);
    _pin = pin;
}

bool TouchPadButton::check(){
    int Current_Button_State = digitalRead(_pin);
    if(Current_Button_State == HIGH && _prev_Button_State != HIGH){
        _prev_Button_State = Current_Button_State;
        return true;
    }else{
        _prev_Button_State = Current_Button_State;
        return false;
    }
}