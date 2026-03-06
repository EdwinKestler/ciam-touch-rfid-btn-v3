/*
  ota.h - OTA update mode setup.
*/
#ifndef OTA_H
#define OTA_H

#include <Arduino.h>

// Enter OTA update mode (blocks in STATE_RDY_TO_UPDATE_OTA)
// Sets fsm_state to STATE_RDY_TO_UPDATE_OTA
void bootToOTA(int& fsm_state);

#endif // OTA_H
