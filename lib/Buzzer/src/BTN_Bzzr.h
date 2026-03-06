/*
  BRN_Bzzr.h - Library for Audible Warning signal using a piezo Buzzer on pin D5.
  Created by Edwin Kestler, JAN 29 , 2019.
  Released into the public domain.
*/

#ifndef BTN_Bzzr_h
#define BTN_Bzzr_h
#include "Arduino.h"

class BTN_Bzzr {
    public:
        BTN_Bzzr(int pin);
        void Beep(unsigned long beep_interval);
        void BeepNonBlocking(unsigned long beep_interval);
        void update();
        bool isBeeping();
    private:
        int _pin;
        int _Bzzr_State;
        unsigned long _beep_interval;
        unsigned long _beep_start;
        bool _beeping;
};

#endif
