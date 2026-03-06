/*
  mqtt_manager.cpp - MQTT connection, reconnection, callback dispatch, topic subscriptions.
*/
#include "mqtt_manager.h"
#include <config.h>
#include <feedback.h>
#include <wifi_setup.h>
#include <ota.h>
#include <Settings.h>
#include <ArduinoJson.h>

// --- Globals ---
WiFiClient wifiClient;
PubSubClient client(wifiClient);

int failed = 0, sent = 0, published = 0;
unsigned long msg_seq = 0;
unsigned long mqtt_backoff_ms = 3000;

String Smacaddrs = "00:00:00:00:00:00";
String Sipaddrs  = "000.000.000.000";

const char LWT_PAYLOAD[] = "{\"d\":{\"Ddata\":{\"Msg\":\"offline\"}}}";

// Forward declaration for fsm_state used by OTA trigger
extern int fsm_state;

// --- MQTT command handlers ---

static void handleResponse(byte* payloadrsp) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, (char*)payloadrsp);
  if (error) {
    Serial.println(F("ERROR en la Letura del JSON Entrante"));
    return;
  }
  Serial.println(F("Response payload:"));
  serializeJson(doc, Serial);
  Serial.println();
}

static void handleUpdate(byte* payload) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, (char*)payload);
  if (error) {
    Serial.println(F("ERROR en la Letura del JSON Entrante"));
    return;
  }
  Serial.println(F("Update payload:"));
  serializeJsonPretty(doc, Serial);
  Serial.println();

  if (doc["publish_interval"].is<unsigned long>()) {
    Universal_1_sec_Interval = doc["publish_interval"];
    Serial.printf("Updated publish_interval: %lu\n", Universal_1_sec_Interval);
  }
  if (doc["btn_hold_time"].is<unsigned long>()) {
    Btn_conf_Mode_Interval = doc["btn_hold_time"];
    Serial.printf("Updated btn_hold_time: %lu\n", Btn_conf_Mode_Interval);
  }
  if (doc["tono_corto"].is<unsigned long>()) {
    tono_corto = doc["tono_corto"];
    Serial.printf("Updated tono_corto: %lu\n", tono_corto);
  }
  if (doc["tono_medio"].is<unsigned long>()) {
    tono_medio = doc["tono_medio"];
    Serial.printf("Updated tono_medio: %lu\n", tono_medio);
  }
  if (doc["tono_largo"].is<unsigned long>()) {
    tono_largo = doc["tono_largo"];
    Serial.printf("Updated tono_largo: %lu\n", tono_largo);
  }
  if (doc["flash_corto"].is<unsigned long>()) {
    flash_corto = doc["flash_corto"];
    Serial.printf("Updated flash_corto: %lu\n", flash_corto);
  }
  if (doc["flash_medio"].is<unsigned long>()) {
    flash_medio = doc["flash_medio"];
    Serial.printf("Updated flash_medio: %lu\n", flash_medio);
  }
  if (doc["flash_largo"].is<unsigned long>()) {
    flash_largo = doc["flash_largo"];
    Serial.printf("Updated flash_largo: %lu\n", flash_largo);
  }
  if (doc["fail_threshold"].is<int>()) {
    fail_threshold = doc["fail_threshold"];
    Serial.printf("Updated fail_threshold: %d\n", fail_threshold);
  }
  if (doc["rssi_threshold"].is<int>()) {
    rssi_low_threshold = doc["rssi_threshold"];
    Serial.printf("Updated rssi_threshold: %d\n", rssi_low_threshold);
  }
  if (doc["heartbeat_minutes"].is<int>()) {
    heartbeat_minutes = constrain((int)doc["heartbeat_minutes"], 1, 1440);
    Serial.printf("Updated heartbeat_minutes: %d\n", heartbeat_minutes);
  }

  if (doc["location"].is<const char*>()) {
    strlcpy(btnconfig.Location, doc["location"], sizeof(btnconfig.Location));
    Serial.printf("Updated location: %s\n", btnconfig.Location);
    saveConfigToLittleFS();
  }

  if (doc["wifi_ssid"].is<const char*>() && doc["wifi_pass"].is<const char*>()) {
    Serial.println(F("Remote WiFi credential update received"));
    strlcpy(btnconfig.Wifi_Fallback_SSID, doc["wifi_ssid"], sizeof(btnconfig.Wifi_Fallback_SSID));
    strlcpy(btnconfig.Wifi_Fallback_Pass, doc["wifi_pass"], sizeof(btnconfig.Wifi_Fallback_Pass));
    saveConfigToLittleFS();
    Serial.printf("Stored new WiFi creds as fallback: %s\n", btnconfig.Wifi_Fallback_SSID);
  }

  if (doc["fallback_ssid"].is<const char*>() && doc["fallback_pass"].is<const char*>()) {
    strlcpy(btnconfig.Wifi_Fallback_SSID, doc["fallback_ssid"], sizeof(btnconfig.Wifi_Fallback_SSID));
    strlcpy(btnconfig.Wifi_Fallback_Pass, doc["fallback_pass"], sizeof(btnconfig.Wifi_Fallback_Pass));
    saveConfigToLittleFS();
    Serial.printf("Updated fallback WiFi: %s\n", btnconfig.Wifi_Fallback_SSID);
  }

  if (doc["ota"].is<bool>() && doc["ota"].as<bool>()) {
    Serial.println(F("Remote OTA trigger received"));
    bootToOTA(fsm_state);
  }
}

static int hexCharToInt(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return 10 + c - 'a';
  if (c >= 'A' && c <= 'F') return 10 + c - 'A';
  return -1;
}

static void handleRGBCommand(byte* payload) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, (char*)payload);
  if (error) {
    Serial.println(F("RGB: JSON parse error"));
    return;
  }

  int r = -1, g = -1, b = -1;

  if (doc["beep"].is<int>() || doc["beep"].is<const char*>()) {
    int seconds = doc["beep"].is<int>() ? (int)doc["beep"] : atoi(doc["beep"].as<const char*>());
    seconds = constrain(seconds, 1, 300);
    Serial.printf("Buzzer: %d seconds\n", seconds);
    alarm_buzzer.BeepNonBlocking((unsigned long)seconds * 1000UL);
    return;
  }

  if (doc["cmd"].is<const char*>()) {
    const char* cmd = doc["cmd"];
    if (strcmp(cmd, "on") == 0) {
      r = 255; g = 255; b = 255;
    } else if (strcmp(cmd, "off") == 0) {
      r = 0; g = 0; b = 0;
    } else {
      Serial.print(F("RGB: unknown cmd: "));
      Serial.println(cmd);
      return;
    }
  }
  else if (doc["hex"].is<const char*>()) {
    const char* hex = doc["hex"];
    if (hex[0] == '#') hex++;
    if (strlen(hex) == 6) {
      r = hexCharToInt(hex[0]) * 16 + hexCharToInt(hex[1]);
      g = hexCharToInt(hex[2]) * 16 + hexCharToInt(hex[3]);
      b = hexCharToInt(hex[4]) * 16 + hexCharToInt(hex[5]);
    } else {
      Serial.println(F("RGB: invalid hex format, expected 6 chars"));
      return;
    }
  }
  else if (doc["r"].is<int>() && doc["g"].is<int>() && doc["b"].is<int>()) {
    r = doc["r"];
    g = doc["g"];
    b = doc["b"];
  }
  else {
    Serial.println(F("RGB: payload must have {r,g,b}, {hex}, or {cmd}"));
    return;
  }

  Rojo.SetPWM(r);
  Verde.SetPWM(g);
  Azul.SetPWM(b);
  Serial.printf("RGB set to R:%d G:%d B:%d\n", r, g, b);
}

// --- MQTT callback dispatcher ---
static void mqttCallback(char* topic, byte* payload, unsigned int payloadLength) {
  Serial.print(F("Mensaje recibido desde el Topico: "));
  Serial.println(topic);

  if (strcmp(responseTopic, topic) == 0) {
    handleResponse(payload);
  }
  if (strcmp(rebootTopic, topic) == 0) {
    JsonDocument rbDoc;
    if (deserializeJson(rbDoc, (char*)payload) == DeserializationError::Ok
        && rbDoc["k"].is<const char*>()
        && strcmp(rbDoc["k"], "A5F0") == 0) {
      Serial.println(F("Reiniciando..."));
      ESP.reset();
    } else {
      Serial.println(F("Reboot: invalid token, ignoring"));
    }
  }
  if (strcmp(updateTopic, topic) == 0) {
    handleUpdate(payload);
  }
  if (strcmp(rgbTopic, topic) == 0) {
    handleRGBCommand(payload);
  }
}

// --- Build MQTT client ID ---
static void buildClientBuf(char* buf, size_t bufSize, const char* clientId, const char* nodeId) {
  snprintf(buf, bufSize, "%s%s", clientId, nodeId);
}

// --- Initial MQTT connection (setup-time) ---
void mqttConnect(const char* clientId, const char* nodeId) {
  client.setServer(btnconfig.MQTT_Server, atoi(btnconfig.MQTT_Port));
  client.setCallback(mqttCallback);
  if (!client.connected()) {
    Serial.print(F("Conectando al servidor MQTT: "));
    Serial.println(btnconfig.MQTT_Server);
    char charBuf[50];
    buildClientBuf(charBuf, sizeof(charBuf), clientId, nodeId);
    for (int retry = 0; retry < 4; retry++) {
      Serial.print(F("MQTT attempt #"));
      Serial.println(retry + 1);
      if (client.connect(charBuf, btnconfig.MQTT_User, btnconfig.MQTT_Password, manageTopic, 0, true, LWT_PAYLOAD)) {
        Serial.println(F("MQTT connected"));
        mqtt_backoff_ms = 3000;
        return;
      }
      Serial.print(F("failed, rc="));
      Serial.println(client.state());
      Blanco.CFlash(flash_corto);
      delay(3000);
    }
    Serial.println(F("MQTT connect failed after 4 attempts"));
    if (tryFallbackWifi()) {
      Serial.println(F("Fallback WiFi connected, retrying MQTT..."));
      Sipaddrs = WiFi.localIP().toString();
      Smacaddrs = String(WiFi.macAddress());
      return;
    }
    Serial.println(F("Opening WiFiManager..."));
    bootToOnDemandWifiManager();
    ESP.restart();
  }
}

// --- Runtime MQTT reconnect with exponential backoff ---
void mqttReconnect(const char* clientId, const char* nodeId) {
  for (int retry = 0; retry < 3 && !client.connected(); retry++) {
    Serial.print(F("Attempting MQTT connection..."));
    feedback_mqtt_retry();
    char charBuf[50];
    buildClientBuf(charBuf, sizeof(charBuf), clientId, nodeId);
    if (client.connect(charBuf, btnconfig.MQTT_User, btnconfig.MQTT_Password, manageTopic, 0, true, LWT_PAYLOAD)) {
      Serial.println(F("connected"));
      mqtt_backoff_ms = 3000;
      initManagedDevice();
      return;
    }
    feedback_mqtt_fail();
    Serial.print(F("failed, rc="));
    Serial.print(client.state());
    Serial.printf(" backoff: %lums\n", mqtt_backoff_ms);
    delay(mqtt_backoff_ms);
    mqtt_backoff_ms = min(mqtt_backoff_ms * 2, MQTT_BACKOFF_MAX);
  }
  if (!client.connected()) {
    Serial.println(F("MQTT reconnect failed after 3 attempts"));
  }
}

// --- Subscribe topics and publish device metadata ---
void initManagedDevice() {
  if (client.subscribe(responseTopic)) {
    Serial.println(F("se ha subscrito al Topico de respuestas"));
  } else {
    Serial.println(F("No se pudo Subscribir al Topico de Respuestas"));
  }

  if (client.subscribe(rebootTopic)) {
    Serial.println(F("se ha subscrito al Topico de Reincio Remoto"));
  } else {
    Serial.println(F("No se pudo Subscribir al Topico de Reinicio Remoto"));
  }

  if (client.subscribe(updateTopic)) {
    Serial.println(F("se ha subscrito al Topico de Actulizaciones Remotas"));
  } else {
    Serial.println(F("No se pudo Subscribir al Topico de Actulizacione Remotas"));
  }

  if (client.subscribe(rgbTopic)) {
    Serial.println(F("se ha subscrito al Topico de Control RGB"));
  } else {
    Serial.println(F("No se pudo Subscribir al Topico de Control RGB"));
  }

  JsonDocument doc;
  JsonObject d = doc["d"].to<JsonObject>();
  JsonObject metadata = d["metadata"].to<JsonObject>();
  metadata["publish_interval"] = Universal_1_sec_Interval;
  metadata["btn_hold_time"] = Btn_conf_Mode_Interval;
  metadata["tono_corto"] = tono_corto;
  metadata["tono_medio"] = tono_medio;
  metadata["tono_largo"] = tono_largo;
  metadata["flash_corto"] = flash_corto;
  metadata["flash_medio"] = flash_medio;
  metadata["flash_largo"] = flash_largo;
  metadata["fail_threshold"] = fail_threshold;
  metadata["rssi_threshold"] = rssi_low_threshold;
  metadata["heartbeat_minutes"] = heartbeat_minutes;
  metadata["timeZone"] = timeZone;
  metadata["location"] = btnconfig.Location;
  metadata["wifi_ssid"] = WiFi.SSID();
  metadata["fallback_ssid"] = btnconfig.Wifi_Fallback_SSID;
  JsonObject supports = d["supports"].to<JsonObject>();
  supports["deviceActions"] = true;
  JsonObject deviceInfo = d["deviceInfo"].to<JsonObject>();
  deviceInfo["NTP_Server"] = btnconfig.NTPClient_SERVER;
  deviceInfo["MQTT_server"] = btnconfig.MQTT_Server;
  deviceInfo["MacAddress"] = Smacaddrs;
  deviceInfo["IPAddress"] = Sipaddrs;
  static char buff[600];
  serializeJson(doc, buff, sizeof(buff));
  Serial.println(F("publishing device manageTopic metadata:"));
  Serial.println(buff);
  sent++;
  if (client.publish(manageTopic, buff)) {
    Serial.println(F("device Publish ok"));
    feedback_publish_ok();
    published++;
  } else {
    Serial.println(F("device Publish failed:"));
    feedback_error();
    failed++;
  }
}
