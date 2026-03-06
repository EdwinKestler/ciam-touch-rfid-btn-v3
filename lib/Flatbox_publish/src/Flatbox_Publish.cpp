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

char * flatbox::Administracion_Dispositivo(const HeartbeatData& data)
{
    JsonDocument doc;
    JsonObject d = doc["d"].to<JsonObject>();
    JsonObject Ddata = d["Ddata"].to<JsonObject>();
    Ddata["ChipID"] = _UID_Board;
    Ddata["DeviceID"] = data.deviceId;
    Ddata["Msg"] = data.status;
    Ddata["FW"] = data.fwVersion;
    Ddata["HW"] = data.hwVersion;
    Ddata["uptime"] = data.uptimeSec;
    Ddata["free_heap"] = data.freeHeap;
    Ddata["hora"] = data.hora;
    Ddata["batt"] = data.battery;
    Ddata["RSSI"] = data.rssi;
    Ddata["SSID"] = data.ssid;
    Ddata["Location"] = data.location;
    Ddata["seq"] = data.seq;
    Ddata["boot_reason"] = data.bootReason;
    Ddata["publicados"] = data.published;
    Ddata["enviados"] = data.sent;
    Ddata["fallidos"] = data.failed;
    Ddata["Tstamp"] = data.timestamp;
    Ddata["Mac"] = data.mac;
    Ddata["Ip"] = data.ip;
    size_t n = serializeJson(doc, _Manejo_Data, sizeof(_Manejo_Data));
    if (n >= sizeof(_Manejo_Data)) {
      Serial.println(F("WARNING: Manejo JSON truncated!"));
    }
    Serial.println(F("publishing device data to manageTopic:"));
    Serial.println(_Manejo_Data);
    return _Manejo_Data;
}

 char * flatbox::Evento_Boton(String Time_Stamp, String ID_Evento_Boton, unsigned long seq)
{
    JsonDocument doc;
    JsonObject d = doc["d"].to<JsonObject>();
    JsonObject botondata = d["botondata"].to<JsonObject>();
    botondata["ChipID"] = _UID_Board;
    botondata["IDEventoBoton"] = ID_Evento_Boton;
    botondata["Tstamp"] = Time_Stamp;
    botondata["seq"] = seq;
    size_t n = serializeJson(doc, _Boton_Data, sizeof(_Boton_Data));
    if (n >= sizeof(_Boton_Data)) {
      Serial.println(F("WARNING: Boton JSON truncated!"));
    }
    Serial.println(F("publishing device publishTopic metadata:"));
    Serial.println(_Boton_Data);
    return _Boton_Data;
}

char * flatbox::Evento_Tarjeta(String ID_Evento_Tarjeta, String Time_Stamp, String ID_Tarjeta_RFID, unsigned long seq)
{
    JsonDocument doc;
    JsonObject d = doc["d"].to<JsonObject>();
    JsonObject tagdata = d["tagdata"].to<JsonObject>();
    tagdata["ChipID"] = _UID_Board;
    tagdata["IDeventoTag"] = ID_Evento_Tarjeta;
    tagdata["Tstamp"] = Time_Stamp;
    tagdata["Tag"] = ID_Tarjeta_RFID;
    tagdata["seq"] = seq;
    size_t n = serializeJson(doc, _Tarjeta_Data, sizeof(_Tarjeta_Data));
    if (n >= sizeof(_Tarjeta_Data)) {
      Serial.println(F("WARNING: Tarjeta JSON truncated!"));
    }
    Serial.println(F("publishing Tag data to publishTopic:"));
    Serial.println(_Tarjeta_Data);
    return _Tarjeta_Data;
}
