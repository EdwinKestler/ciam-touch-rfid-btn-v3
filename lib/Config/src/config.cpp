/*
  config.cpp - Device configuration: LittleFS read/save, XOR obfuscation, WiFiManager param handling.
*/
#include "config.h"

BtnConf btnconfig;
bool shouldSaveConfig = false;

// --- XOR hex obfuscation ---
static const char HEX_CHARS[] = "0123456789abcdef";

void xorObfuscateHex(const char* input, char* output, size_t outLen) {
  size_t j = 0;
  for (size_t i = 0; input[i] != '\0' && j + 2 < outLen; i++) {
    uint8_t b = (uint8_t)input[i] ^ XOR_KEY;
    output[j++] = HEX_CHARS[b >> 4];
    output[j++] = HEX_CHARS[b & 0x0F];
  }
  output[j] = '\0';
}

void xorDeobfuscateHex(const char* hexInput, char* output, size_t outLen) {
  size_t len = strlen(hexInput);
  size_t j = 0;
  for (size_t i = 0; i + 1 < len && j + 1 < outLen; i += 2) {
    uint8_t hi = (hexInput[i] >= 'a') ? (hexInput[i] - 'a' + 10) : (hexInput[i] - '0');
    uint8_t lo = (hexInput[i+1] >= 'a') ? (hexInput[i+1] - 'a' + 10) : (hexInput[i+1] - '0');
    output[j++] = (char)((hi << 4 | lo) ^ XOR_KEY);
  }
  output[j] = '\0';
}

// --- WiFiManager callbacks ---
void saveConfigCallback() {
  Serial.println(F("Deberia Guardar la configuracion"));
  shouldSaveConfig = true;
}

// --- Read config from LittleFS ---
void readConfigFromLittleFS() {
  Serial.println(F("Montando el Sistema de Archivos o FS"));
  if (LittleFS.begin()) {
    Serial.println(F("Sistema de Archivos montado"));
    if (LittleFS.exists("/config.json")) {
      Serial.println(F("Leyendo el Archivo de Configuracion"));
      File configFile = LittleFS.open("/config.json", "r");
      if (configFile) {
        Serial.println(F("Archivo de configuracion Abierto"));
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, buf.get());
        if (error) {
          Serial.println(F("Fallo en leer el archivo"));
          configFile.close();
          return;
        }
        serializeJson(doc, Serial);
        Serial.println(F("\nparsed json"));

        strlcpy(btnconfig.MQTT_Server, doc["MQTT_Server"] | "172.18.98.142", sizeof(btnconfig.MQTT_Server));
        strlcpy(btnconfig.MQTT_Port, doc["MQTT_Port"] | "1883", sizeof(btnconfig.MQTT_Port));
        strlcpy(btnconfig.MQTT_User, doc["MQTT_User"] | "esp8266", sizeof(btnconfig.MQTT_User));
        strlcpy(btnconfig.MQTT_Password, doc["MQTT_Password"] | "esp8266", sizeof(btnconfig.MQTT_Password));
        strlcpy(btnconfig.NTPClient_SERVER, doc["NTPClient_SERVER"] | "time-a-g.nist.gov", sizeof(btnconfig.NTPClient_SERVER));
        strlcpy(btnconfig.NTPClient_interval, doc["NTPClient_interval"] | "60000", sizeof(btnconfig.NTPClient_interval));
        strlcpy(btnconfig.Device_ID, doc["Device_ID"] | "CIAM", sizeof(btnconfig.Device_ID));
        strlcpy(btnconfig.Location, doc["Location"] | "", sizeof(btnconfig.Location));
        strlcpy(btnconfig.Wifi_Fallback_SSID, doc["Wifi_Fallback_SSID"] | "", sizeof(btnconfig.Wifi_Fallback_SSID));
        strlcpy(btnconfig.Wifi_Fallback_Pass, doc["Wifi_Fallback_Pass"] | "", sizeof(btnconfig.Wifi_Fallback_Pass));

        // Config version migration
        int stored_version = doc["config_version"] | 0;
        if (stored_version >= 3) {
          char hex_pass[48];
          strlcpy(hex_pass, doc["MQTT_Password"] | "", sizeof(hex_pass));
          xorDeobfuscateHex(hex_pass, btnconfig.MQTT_Password, sizeof(btnconfig.MQTT_Password));
        } else if (stored_version == 2) {
          Serial.println(F("Config v2 detected: password corrupted by binary XOR, using default"));
          strlcpy(btnconfig.MQTT_Password, "esp8266", sizeof(btnconfig.MQTT_Password));
        }
        // v0/v1: password already read as plaintext above
        if (stored_version < CONFIG_VERSION) {
          Serial.printf("Config migration: v%d -> v%d\n", stored_version, CONFIG_VERSION);
          shouldSaveConfig = true;
        }

        configFile.close();
      }
    }
  } else {
    Serial.println(F("Fallo en Abrir el Sistema de Archivos"));
  }
}

// --- Save config to LittleFS ---
void saveConfigToLittleFS() {
  JsonDocument doc;
  doc["MQTT_Server"] = btnconfig.MQTT_Server;
  doc["MQTT_Port"] = btnconfig.MQTT_Port;
  doc["MQTT_User"] = btnconfig.MQTT_User;
  { char obf[48]; xorObfuscateHex(btnconfig.MQTT_Password, obf, sizeof(obf)); doc["MQTT_Password"] = obf; }
  doc["NTPClient_SERVER"] = btnconfig.NTPClient_SERVER;
  doc["NTPClient_interval"] = btnconfig.NTPClient_interval;
  doc["Device_ID"] = btnconfig.Device_ID;
  doc["Location"] = btnconfig.Location;
  doc["Wifi_Fallback_SSID"] = btnconfig.Wifi_Fallback_SSID;
  doc["Wifi_Fallback_Pass"] = btnconfig.Wifi_Fallback_Pass;
  doc["config_version"] = CONFIG_VERSION;

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println(F("ERROR: no se pudo abrir config.json para escritura"));
    return;
  }
  serializeJson(doc, configFile);
  configFile.close();
  Serial.println(F("config.json guardado"));
}

// --- WiFiManager param setup ---
void setupWifiManagerParams(WiFiManager &wm,
    WiFiManagerParameter &srv, WiFiManagerParameter &port, WiFiManagerParameter &usr,
    WiFiManagerParameter &pwd, WiFiManagerParameter &ntp_srv, WiFiManagerParameter &ntp_int,
    WiFiManagerParameter &dev_id, WiFiManagerParameter &loc,
    WiFiManagerParameter &fb_ssid, WiFiManagerParameter &fb_pass)
{
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.addParameter(&srv);
  wm.addParameter(&port);
  wm.addParameter(&usr);
  wm.addParameter(&pwd);
  wm.addParameter(&ntp_int);
  wm.addParameter(&ntp_srv);
  wm.addParameter(&dev_id);
  wm.addParameter(&loc);
  wm.addParameter(&fb_ssid);
  wm.addParameter(&fb_pass);
}

// --- Copy WiFiManager values to btnconfig and save ---
void copyWifiManagerParams(
    WiFiManagerParameter &srv, WiFiManagerParameter &port, WiFiManagerParameter &usr,
    WiFiManagerParameter &pwd, WiFiManagerParameter &ntp_srv, WiFiManagerParameter &ntp_int,
    WiFiManagerParameter &dev_id, WiFiManagerParameter &loc,
    WiFiManagerParameter &fb_ssid, WiFiManagerParameter &fb_pass)
{
  strlcpy(btnconfig.MQTT_Server, srv.getValue(), sizeof(btnconfig.MQTT_Server));
  strlcpy(btnconfig.MQTT_Port, port.getValue(), sizeof(btnconfig.MQTT_Port));
  strlcpy(btnconfig.MQTT_User, usr.getValue(), sizeof(btnconfig.MQTT_User));
  strlcpy(btnconfig.MQTT_Password, pwd.getValue(), sizeof(btnconfig.MQTT_Password));
  strlcpy(btnconfig.NTPClient_SERVER, ntp_srv.getValue(), sizeof(btnconfig.NTPClient_SERVER));
  strlcpy(btnconfig.NTPClient_interval, ntp_int.getValue(), sizeof(btnconfig.NTPClient_interval));
  strlcpy(btnconfig.Device_ID, dev_id.getValue(), sizeof(btnconfig.Device_ID));
  strlcpy(btnconfig.Location, loc.getValue(), sizeof(btnconfig.Location));
  strlcpy(btnconfig.Wifi_Fallback_SSID, fb_ssid.getValue(), sizeof(btnconfig.Wifi_Fallback_SSID));
  strlcpy(btnconfig.Wifi_Fallback_Pass, fb_pass.getValue(), sizeof(btnconfig.Wifi_Fallback_Pass));

  if (shouldSaveConfig) {
    Serial.println(F("Guardando Cambios de Configuracion"));
    saveConfigToLittleFS();
  }
}
