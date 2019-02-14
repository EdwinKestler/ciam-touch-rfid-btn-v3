/*
  Flatbox_Publish.h - Library for the publishing of data to MQTT Flatbox Services.
  Created by Edwin Kestler, JAN 13 , 2019.
  Released into the public domain uner MIT Licence.
*/

#ifndef Flatbox_Publish_h
#define Flatbox_Publish_h
#include "Arduino.h"

class flatbox {
    public:
        flatbox(String UID_Board, char * This_Topic);
        bool Administracion_Dispositivo(String Mensaje_Estado, float Voltaje_Board, int Nivel_RSSI, int Mensajes_enviados, int Mensajes_Fallidos, String Time_Stamp, String Direccion_Mac, String Direccion_IP);
        bool Evento_Boton(String Time_Stamp, String ID_Evento_Boton);
        bool Evento_Tarjeta(String ID_Evento_Tarjeta, String Time_Stamp, String ID_Tarjeta_RFID);
    private:
        String _UID_Board;
        char * _This_Topic;
};

#endif
