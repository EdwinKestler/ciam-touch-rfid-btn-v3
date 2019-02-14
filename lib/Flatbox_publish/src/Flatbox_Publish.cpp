/*
  Flatbox_Publish.h - Library for the publishing of data to MQTT Flatbox Services.
  Created by Edwin Kestler, JAN 13 , 2019.
  Released into the public domain uner MIT Licence.
*/

#include "Arduino.h"
#include "Flatbox_Publish.h"
#include <ArduinoJson.h>

flatbox::flatbox(String UID_Board){
    _UID_Board = UID_Board;
}

char * flatbox::Administracion_Dispositivo(String Mensaje_Estado, float Voltaje_Board, int Nivel_RSSI, int Mensajes_enviados, int Mensajes_Fallidos, String Time_Stamp, String Direccion_Mac, String Direccion_IP)
{
    StaticJsonBuffer<300> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    JsonObject& d = root.createNestedObject("d");
    JsonObject& Ddata = d.createNestedObject("Ddata");
    Ddata["ChipID"] = _UID_Board;
    Ddata["Msg"] = Mensaje_Estado;
    Ddata["batt"] = Voltaje_Board;
    Ddata["RSSI"] = Nivel_RSSI;
    Ddata["publicados"] = Mensajes_enviados;
    Ddata["enviados"] = Mensajes_enviados;
    Ddata["fallidos"] = Mensajes_Fallidos;
    Ddata["Tstamp"] = Time_Stamp;
    Ddata["Mac"] = Direccion_Mac;
    Ddata["Ip"] = Direccion_IP;
    char MqttDevicedata[300];
    root.printTo(MqttDevicedata, sizeof(MqttDevicedata));
    Serial.println(F("publishing device data to manageTopic:"));
    Serial.println(MqttDevicedata);
    strcpy(_Manejo_Data, MqttDevicedata);
    return _Manejo_Data;
}

 char * flatbox::Evento_Boton(String Time_Stamp, String ID_Evento_Boton)
{
    StaticJsonBuffer<300> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    JsonObject& d = root.createNestedObject("d");
    JsonObject& botondata = d.createNestedObject("botondata");
    botondata["ChipID"] = _UID_Board;
    botondata["IDEventoBoton"] = ID_Evento_Boton;
    botondata["Tstamp"] = Time_Stamp;
    char MqttBotondata[300];
    root.printTo(MqttBotondata, sizeof(MqttBotondata));
    Serial.println(F("publishing device publishTopic metadata:"));
    Serial.println(MqttBotondata);
    strcpy(_Boton_Data, MqttBotondata);
    return _Boton_Data;
}

char * flatbox::Evento_Tarjeta(String ID_Evento_Tarjeta, String Time_Stamp, String ID_Tarjeta_RFID)
{
    StaticJsonBuffer<300> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    JsonObject& d = root.createNestedObject("d");
    JsonObject& tagdata = d.createNestedObject("tagdata");
    tagdata["ChipID"] = _UID_Board;
    tagdata["IDeventoTag"] = ID_Evento_Tarjeta;
    tagdata["Tstamp"] = Time_Stamp;
    tagdata["Tag"] = ID_Tarjeta_RFID;
    char MqttTagdata[300];
    root.printTo(MqttTagdata, sizeof(MqttTagdata));
    Serial.println(F("publishing Tag data to publishTopic:"));
    Serial.println(MqttTagdata);
    strcpy(_Tarjeta_Data, MqttTagdata);
    return _Tarjeta_Data;
}