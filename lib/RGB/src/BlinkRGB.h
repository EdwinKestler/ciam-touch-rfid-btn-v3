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
        void FlashNonBlocking(unsigned long flash_interval);
        void update();
        void This_RGB_State(int RGB_STATE);
        void SetPWM(int value);
    private:
        int _pin;
        int _RGB_State;
        unsigned long _flash_interval;
        unsigned long _flash_start;
        bool _flashing;
};

class BlinkColor {
    public:
        BlinkColor(int pin0, int pin1, int pin2);
        void COn();
        void COff();
        void CFlash(unsigned long Cflash_interval);
        void CFlashNonBlocking(unsigned long Cflash_interval);
        void update();
        void SetRGB(int r, int g, int b);
    private:
        int _pin0;
        int _pin1;
        int _pin2;
        unsigned long _Cflash_interval;
        unsigned long _flash_start;
        bool _flashing;
};

#endif