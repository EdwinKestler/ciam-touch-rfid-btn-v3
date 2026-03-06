/*
  Flatbox_Publish.h - Library for the publishing of data to MQTT Flatbox Services.
  Created by Edwin Kestler, JAN 13 , 2019.
  Released into the public domain uner MIT Licence.
*/

#ifndef Flatbox_Publish_h
#define Flatbox_Publish_h
#include "Arduino.h"

struct HeartbeatData {
    const char* status;
    float battery;
    int rssi;
    int published;
    int sent;
    int failed;
    const char* timestamp;
    const char* mac;
    const char* ip;
    const char* deviceId;
    const char* fwVersion;
    const char* hwVersion;
    int hora;
    unsigned long uptimeSec;
    uint32_t freeHeap;
    const char* ssid;
    const char* location;
    unsigned long seq;
    const char* bootReason;
};

class flatbox {
    public:
        flatbox(String UID_Board);
        char * Administracion_Dispositivo(const HeartbeatData& data);
        char * Evento_Boton(String Time_Stamp, String ID_Evento_Boton, unsigned long seq);
        char * Evento_Tarjeta(String ID_Evento_Tarjeta, String Time_Stamp, String ID_Tarjeta_RFID, unsigned long seq);
        char * Evento_Power(String Time_Stamp, float voltage, int adcRaw, unsigned long seq);
    private:
        String _UID_Board;
        char _Boton_Data[300];
        char _Tarjeta_Data[300];
        char _Power_Data[300];
        char _Manejo_Data[600];
};

#endif
