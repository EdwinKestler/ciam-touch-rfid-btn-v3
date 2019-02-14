/*
  Flatbox_Publish.h - Library for the publishing of data to MQTT Flatbox Services.
  Created by Edwin Kestler, JAN 13 , 2019.
  Released into the public domain uner MIT Licence.
*/

#include "Arduino.h"
#include "Flatbox_Publish.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

WiFiClient Wifi_Client;                                                                               //Se establece el Cliente Wifi
PubSubClient MQTT_Client(Wifi_Client);                                                                    //se establece el Cliente para el servicio MQTT


flatbox::flatbox(String UID_Board, char* This_Topic){
    _This_Topic = This_Topic;
    _UID_Board = UID_Board;
}

bool flatbox::Administracion_Dispositivo(String Mensaje_Estado, float Voltaje_Board, int Nivel_RSSI, int Mensajes_enviados, int Mensajes_Fallidos, String Time_Stamp, String Direccion_Mac, String Direccion_IP)
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
    //return MqttDevicedata;
    if (MQTT_Client.publish(_This_Topic, MqttDevicedata)) {
        Serial.println(F("enviado data de dispositivo:OK"));
        return true;
    } else {
        Serial.print(F("enviado data de dispositivo:FAILED"));
        return false;
    }
}

bool flatbox::Evento_Boton(String Time_Stamp, String ID_Evento_Boton)
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
    if (MQTT_Client.publish(_This_Topic, MqttBotondata)) {
        Serial.println(F("enviado data de boton: OK"));
        return true;
    }else {
        Serial.println(F("enviado data de boton: FAILED"));
        return false;
    }
}

bool flatbox::Evento_Tarjeta(String ID_Evento_Tarjeta, String Time_Stamp, String ID_Tarjeta_RFID)
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
    if (MQTT_Client.publish(_This_Topic, MqttTagdata)) {
        Serial.println(F("enviado data de RFID: OK"));
        return true;
    } else {
        Serial.println(F("enviado data de RFID: FAILED"));
        return false;
    }
}