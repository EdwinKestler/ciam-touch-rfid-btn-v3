/*
  BlinkRGB.h - Library for RGB visual signal.
  Created by Edwin Kestler, May 29 , 2018.
  Released into the public domain.
*/

#ifndef BlinkRGB_h
#define BlinkRGB_h
#include "Arduino.h"

class BlinkRGB {
    public:
        BlinkRGB(int pin);
        void Flash(unsigned long flash_interval);
        void This_RGB_State(int RGB_STATE);
        void Change_RGB_State();
    private:
        int _pin;
        int _RGB_State;
        unsigned long _flash_interval;
};

class BlinkColor {
    public:
        BlinkColor(int pin0, int pin1, int pin2);
        void COn();
        void COff();
        void CFlash(unsigned long Cflash_interval);
    private:
        int _pin0;
        int _pin1;
        int _pin2;
        unsigned long _Cflash_interval;
};

#endif