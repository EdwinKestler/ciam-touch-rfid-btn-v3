/*
  feedback.cpp - Unified audio/visual feedback layer.
*/
#include "feedback.h"

// Durations (remotely configurable)
unsigned long tono_corto = 250;
unsigned long tono_medio = 500;
unsigned long tono_largo = 1000;
unsigned long flash_corto = 250;
unsigned long flash_medio = 500;
unsigned long flash_largo = 1000;

// Hardware objects
BlinkRGB Azul(PIN_LED_BLUE);
BlinkRGB Verde(PIN_LED_GREEN);
BlinkRGB Rojo(PIN_LED_RED);
BlinkColor Blanco(PIN_LED_BLUE, PIN_LED_GREEN, PIN_LED_RED);
BlinkColor Purpura(PIN_LED_BLUE, PIN_LED_PURPLE, PIN_LED_RED);
BTN_Bzzr alarm_buzzer(PIN_BUZZER);

void feedback_ok() {
    Verde.Flash(flash_corto);
    alarm_buzzer.Beep(tono_corto);
}

void feedback_error() {
    Rojo.Flash(flash_corto);
    alarm_buzzer.Beep(tono_medio);
}

void feedback_warning() {
    Rojo.Flash(flash_medio);
    alarm_buzzer.Beep(tono_medio);
}

void feedback_alarm() {
    Blanco.CFlash(flash_largo);
    alarm_buzzer.Beep(tono_largo);
}

void feedback_publish_ok() {
    Verde.Flash(flash_corto);
    alarm_buzzer.Beep(tono_corto);
}

void feedback_publish_fail() {
    Rojo.Flash(flash_corto);
}

void feedback_mqtt_retry() {
    Blanco.CFlash(flash_corto);
    alarm_buzzer.Beep(tono_corto);
}

void feedback_mqtt_fail() {
    Purpura.CFlash(flash_medio);
    alarm_buzzer.Beep(tono_medio);
}

void feedback_card_read() {
    Verde.Flash(flash_corto);
    alarm_buzzer.Beep(tono_corto);
}

void feedback_button_press() {
    Azul.Flash(flash_corto);
    alarm_buzzer.Beep(tono_corto);
}

void feedback_update() {
    alarm_buzzer.update();
}
