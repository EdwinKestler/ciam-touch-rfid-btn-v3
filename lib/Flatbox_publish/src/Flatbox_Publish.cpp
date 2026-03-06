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

char * flatbox::Administracion_Dispositivo(String Mensaje_Estado, float Voltaje_Board, int Nivel_RSSI, int Mensajes_publicados, int Mensajes_enviados, int Mensajes_Fallidos, String Time_Stamp, String Direccion_Mac, String Direccion_IP, const char* device_id, const char* fw_version, const char* hw_version, int hora, unsigned long uptime_sec, uint32_t free_heap, const char* ssid, const char* location)
{
    JsonDocument doc;
    JsonObject d = doc["d"].to<JsonObject>();
    JsonObject Ddata = d["Ddata"].to<JsonObject>();
    Ddata["ChipID"] = _UID_Board;
    Ddata["DeviceID"] = device_id;
    Ddata["Msg"] = Mensaje_Estado;
    Ddata["FW"] = fw_version;
    Ddata["HW"] = hw_version;
    Ddata["uptime"] = uptime_sec;
    Ddata["free_heap"] = free_heap;
    Ddata["hora"] = hora;
    Ddata["batt"] = Voltaje_Board;
    Ddata["RSSI"] = Nivel_RSSI;
    Ddata["SSID"] = ssid;
    Ddata["Location"] = location;
    Ddata["publicados"] = Mensajes_publicados;
    Ddata["enviados"] = Mensajes_enviados;
    Ddata["fallidos"] = Mensajes_Fallidos;
    Ddata["Tstamp"] = Time_Stamp;
    Ddata["Mac"] = Direccion_Mac;
    Ddata["Ip"] = Direccion_IP;
    size_t n = serializeJson(doc, _Manejo_Data, sizeof(_Manejo_Data));
    if (n >= sizeof(_Manejo_Data)) {
      Serial.println(F("WARNING: Manejo JSON truncated!"));
    }
    Serial.println(F("publishing device data to manageTopic:"));
    Serial.println(_Manejo_Data);
    return _Manejo_Data;
}

 char * flatbox::Evento_Boton(String Time_Stamp, String ID_Evento_Boton)
{
    JsonDocument doc;
    JsonObject d = doc["d"].to<JsonObject>();
    JsonObject botondata = d["botondata"].to<JsonObject>();
    botondata["ChipID"] = _UID_Board;
    botondata["IDEventoBoton"] = ID_Evento_Boton;
    botondata["Tstamp"] = Time_Stamp;
    size_t n = serializeJson(doc, _Boton_Data, sizeof(_Boton_Data));
    if (n >= sizeof(_Boton_Data)) {
      Serial.println(F("WARNING: Boton JSON truncated!"));
    }
    Serial.println(F("publishing device publishTopic metadata:"));
    Serial.println(_Boton_Data);
    return _Boton_Data;
}

char * flatbox::Evento_Tarjeta(String ID_Evento_Tarjeta, String Time_Stamp, String ID_Tarjeta_RFID)
{
    JsonDocument doc;
    JsonObject d = doc["d"].to<JsonObject>();
    JsonObject tagdata = d["tagdata"].to<JsonObject>();
    tagdata["ChipID"] = _UID_Board;
    tagdata["IDeventoTag"] = ID_Evento_Tarjeta;
    tagdata["Tstamp"] = Time_Stamp;
    tagdata["Tag"] = ID_Tarjeta_RFID;
    size_t n = serializeJson(doc, _Tarjeta_Data, sizeof(_Tarjeta_Data));
    if (n >= sizeof(_Tarjeta_Data)) {
      Serial.println(F("WARNING: Tarjeta JSON truncated!"));
    }
    Serial.println(F("publishing Tag data to publishTopic:"));
    Serial.println(_Tarjeta_Data);
    return _Tarjeta_Data;
}
