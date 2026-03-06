/*
  main.cpp - CIAM Touch RFID Button platform
  FSM, setup, loop, and sensor read/publish functions.
  All infrastructure (config, MQTT, WiFi, feedback, OTA) is in separate modules.
*/
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <TimeLibEsp.h>
#include <NTP_Client.h>
#include <TouchPadButton.h>
#include <Flatbox_Publish.h>
#include <Settings.h>

#include <pins.h>
#include <config.h>
#include <feedback.h>
#include <ota.h>
#include <wifi_setup.h>
#include <mqtt_manager.h>

// --- FSM states ---
enum FsmState {
  STATE_IDLE                   = 0,
  STATE_TRANSMIT_BOTON_DATA    = 1,
  STATE_TRANSMIT_CARD_DATA     = 2,
  STATE_UPDATE                 = 3,
  STATE_TRANSMIT_DEVICE_UPDATE = 5,
  STATE_UPDATE_TIME            = 6,
  STATE_RDY_TO_UPDATE_OTA      = 7
};

int fsm_state;

// --- OTA / WiFiManager boot flags ---
bool OTA_ENABLED = false;
bool Wifi_On_Demand_ENABLED = false;
unsigned long Btn_check_Current_millis;

// --- RFID reader ---
SoftwareSerial RFIDReader(PIN_RFID_TX, PIN_RFID_RX, false);
#define RFID_FRAME_SIZE 13
#define RFID_RATE_LIMIT_MS 60000UL

byte incomingdata;
String inputString;
byte tagID[RFID_FRAME_SIZE];
String OldTagRead = "1";
boolean readedTag = false;
unsigned int count = 0;
unsigned long RetardoLectura;
unsigned long lastCardPublishMillis = 0;
String lastPublishedCardID = "";
int Numero_ID_Eventos_Tarjeta = 0;

// --- Touch button ---
TouchPadButton T_button(PIN_TOUCH);
char identificador_ID_Evento_Boton[32];
int Numero_ID_Evento_Boton = 0;

// --- ESP8266 identity ---
String NodeID = String(ESP.getChipId());
char clientId[50];

// --- NTP ---
WiFiUDP ntpUDP;
boolean NTP_response = false;
char ISO8601[21] = "";
NTPClient* pTimeClient = nullptr;

// --- Flatbox JSON publisher ---
flatbox Flatbox_Json(NodeID);

// --- Timers and status ---
unsigned long Last_Normal_Reset_Millis;
unsigned long Last_Update_Millis;
unsigned long Last_NTP_Update_Millis;
char msg[20] = "";
int BeepSignalWarning = 0;
float VBat = 0;
int hora = 0;
int WifiSignal;
String bootReason;

// ============================================================ NTP
time_t NTP_ready() {
  Serial.println(F("Transmit NTP Request"));
  pTimeClient->update();
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    Serial.print(".");
    delay(100);
    unsigned long EpochTime = pTimeClient->getRawTime();
    if (EpochTime >= 3610) {
      Serial.println();
      Serial.println(F("Receive NTP Response"));
      NTP_response = true;
      return EpochTime + (timeZone * SECS_PER_HOUR);
    }
  }
  Serial.println();
  Serial.println(F("No NTP Response :-("));
  NTP_response = false;
  return 0;
}

void CheckTime() {
  static time_t prevDisplay = 0;
  if (timeStatus() != timeNotSet) {
    time_t t = now();
    if (t != prevDisplay) {
      prevDisplay = t;
      snprintf(ISO8601, sizeof(ISO8601), "%04d-%02d-%02dT%02d:%02d:%02d", year(), month(), day(), hour(), minute(), second());
    }
  } else {
    Serial.println(F("Time not Sync, Syncronizing time"));
    NTP_ready();
  }
}

// ============================================================ RFID
void clearBufferArray() {
  inputString = "";
  for (unsigned int i = 0; i < RFID_FRAME_SIZE; i++) {
    tagID[i] = 0;
  }
}

void readTag() {
  if (RFIDReader.available()) {
    inputString = "";
    while (RFIDReader.available() > 0) {
      incomingdata = RFIDReader.read();
      tagID[count] = incomingdata;
      if (count > 3 && count < 8) {
        inputString += incomingdata;
      }
      delay(2);
      if (count == 12) break;
      count++;
    }
    count = 0;
    bool allZero = true;
    for (unsigned int i = 0; i < inputString.length(); i++) {
      if (inputString[i] != '0' && inputString[i] != '\0') { allZero = false; break; }
    }
    if (inputString.length() == 0 || allZero) {
      Serial.println(F("RFID: invalid frame, discarding"));
      clearBufferArray();
      return;
    }
    Serial.print(F("RFID CARD ID IS: "));
    Serial.println(inputString);
    if (OldTagRead == inputString) {
      Serial.println(F("Duplicate read, ignoring"));
      return;
    }
    feedback_card_read();
    readedTag = !readedTag;
    fsm_state = STATE_TRANSMIT_CARD_DATA;
  }
}

// ============================================================ Button
void readBtn() {
  if (T_button.check()) {
    Serial.println(F("Pressed"));
    Numero_ID_Evento_Boton++;
    snprintf(identificador_ID_Evento_Boton, sizeof(identificador_ID_Evento_Boton), "%s-%d", NodeID.c_str(), Numero_ID_Evento_Boton);
    feedback_button_press();
    fsm_state = STATE_TRANSMIT_BOTON_DATA;
  }
}

// ============================================================ Publish functions
void publishRF_ID_Manejo() {
  sent++;
  msg_seq++;
  char* payload = Flatbox_Json.Administracion_Dispositivo(msg, VBat, WifiSignal, published, sent, failed, ISO8601, Smacaddrs, Sipaddrs, btnconfig.Device_ID, FirmwareVersion, HardwareVersion, hora, millis()/1000, ESP.getFreeHeap(), WiFi.SSID().c_str(), btnconfig.Location, msg_seq, bootReason.c_str());
  if (client.publish(manageTopic, payload, true)) {
    Serial.println(F("enviado data de dispositivo:OK"));
    published++;
    failed = failed / 2;
  } else {
    Serial.print(F("enviado data de dispositivo:FAILED"));
    failed++;
  }
}

void publish_Boton_Data() {
  sent++;
  msg_seq++;
  if (client.publish(publishTopic, Flatbox_Json.Evento_Boton(ISO8601, identificador_ID_Evento_Boton, msg_seq))) {
    Serial.println(F("enviado data de boton: OK"));
    feedback_publish_ok();
    published++;
    failed = failed / 2;
  } else {
    Serial.println(F("enviado data de boton: FAILED"));
    feedback_publish_fail();
    failed++;
  }
  Blanco.COff();
}

boolean publishRF_ID_Lectura() {
  if (OldTagRead != inputString) {
    if (inputString == lastPublishedCardID && millis() - lastCardPublishMillis < RFID_RATE_LIMIT_MS) {
      Serial.println(F("RFID: rate limited, same card too soon"));
      OldTagRead = inputString;
      inputString = "";
      return false;
    }
    OldTagRead = inputString;
    lastPublishedCardID = inputString;
    lastCardPublishMillis = millis();
    Numero_ID_Eventos_Tarjeta++;
    char Identificador_ID_Evento_Tarjeta[32];
    snprintf(Identificador_ID_Evento_Tarjeta, sizeof(Identificador_ID_Evento_Tarjeta), "%s-%d", NodeID.c_str(), Numero_ID_Eventos_Tarjeta);
    sent++;
    msg_seq++;
    if (client.publish(publishTopic, Flatbox_Json.Evento_Tarjeta(Identificador_ID_Evento_Tarjeta, ISO8601, inputString, msg_seq))) {
      Serial.println(F("enviado data de RFID: OK"));
      feedback_publish_ok();
      published++;
      inputString = "";
      failed = failed / 2;
      return true;
    } else {
      Serial.println(F("enviado data de RFID: FAILED"));
      feedback_publish_fail();
      failed++;
      OldTagRead = "1";
      inputString = "";
      return false;
    }
  } else {
    Serial.println("Este es una lectura consecutiva");
    return false;
  }
}

// ============================================================ Periodic functions
void NormalReset() {
  if (millis() - Last_Normal_Reset_Millis > 60 * 60 * Universal_1_sec_Interval) {
    hora++;
    WifiSignal = WiFi.RSSI();
    if (hora > 24) {
      strlcpy(msg, "24h Normal Reset", sizeof(msg));
      if (!client.connected()) { mqttReconnect(clientId, NodeID.c_str()); }
      CheckTime();
      publishRF_ID_Manejo();
      client.disconnect();
      hora = 0;
      ESP.restart();
    }
    Last_Normal_Reset_Millis = millis();
  }
}

void checkalarms() {
  if (WiFi.RSSI() < rssi_low_threshold) {
    if (BeepSignalWarning < 4) {
      feedback_alarm();
      BeepSignalWarning++;
    }
  } else {
    BeepSignalWarning = 0;
  }
}

void updateDeviceInfo() {
  strlcpy(msg, "on", sizeof(msg));
  WifiSignal = WiFi.RSSI();
  if (WiFi.RSSI() < rssi_low_threshold) {
    strlcpy(msg, "LOWiFi", sizeof(msg));
    feedback_warning();
    Serial.print(WiFi.SSID());
    Serial.print(" ");
    Serial.println(WiFi.RSSI());
    fsm_state = STATE_TRANSMIT_DEVICE_UPDATE;
    return;
  }
}

// ============================================================ SETUP
void setup() {
  ESP.wdtEnable(8000);
  bootReason = ESP.getResetReason();
  Blanco.COff();

  Serial.begin(115200);
  Serial.print(F("Boot reason: "));
  Serial.println(bootReason);
  Serial.println(F("inicio exitosamnte el puerto Serial"));
  Serial.println();

  RFIDReader.begin(9600);

  // Load config from LittleFS
  readConfigFromLittleFS();
  if (shouldSaveConfig) {
    saveConfigToLittleFS();
    shouldSaveConfig = false;
  }

  // Build MQTT client ID from config
  snprintf(clientId, sizeof(clientId), "d:%s:%s:%s", ORG, DEVICE_TYPE, btnconfig.Device_ID);

  // Initialize NTP client
  pTimeClient = new NTPClient(btnconfig.NTPClient_SERVER, 0, atoi(btnconfig.NTPClient_interval));

  // --- Boot mode selection: button held during startup windows ---
  Serial.print(F("            estado del Boton: "));
  Serial.println(T_button.check());
  delay(Universal_1_sec_Interval);

  // Window 1: Hold button for on-demand WiFi config portal
  Btn_check_Current_millis = millis();
  Verde.This_RGB_State(HIGH);
  while (millis() < (Btn_check_Current_millis + Btn_conf_Mode_Interval) && !OTA_ENABLED) {
    if (T_button.check()) {
      bootToOnDemandWifiManager();
      Wifi_On_Demand_ENABLED = true;
    }
    delay(11);
  }
  Verde.This_RGB_State(LOW);

  // Window 2: Hold button for OTA update mode
  Rojo.This_RGB_State(HIGH);
  Btn_check_Current_millis = millis();
  while (millis() < (Btn_check_Current_millis + Btn_conf_Mode_Interval) && !Wifi_On_Demand_ENABLED) {
    if (T_button.check()) {
      bootToOTA(fsm_state);
      OTA_ENABLED = true;
    }
    delay(11);
  }
  Rojo.This_RGB_State(LOW);

  // Normal boot: WiFi -> NTP -> MQTT
  if (!OTA_ENABLED) {
    bootToWifiManager();
    if (WiFi.status() != WL_CONNECTED) {
      if (!tryFallbackWifi()) {
        while (WiFi.status() != WL_CONNECTED) {
          bootToWifiManager();
          delay(Universal_1_sec_Interval);
          Serial.print(".");
        }
      }
    }
    Serial.print(F("Wifi conectado, Direccion de IP Asignado: "));
    Serial.println(WiFi.localIP());
    Sipaddrs = WiFi.localIP().toString();
    Serial.print(F("Direccion de MAC Asignado: "));
    Serial.println(WiFi.macAddress());
    Smacaddrs = String(WiFi.macAddress());
    Serial.println();

    // NTP sync
    Serial.print(F("servidor de NTP: "));
    Serial.println(btnconfig.NTPClient_SERVER);
    Serial.print(F("Intervalo de actualizacion: "));
    Serial.println(atoi(btnconfig.NTPClient_interval));
    pTimeClient->begin();

    if (WiFi.status() == WL_CONNECTED) {
      for (int ntp_retry = 0; ntp_retry < 5 && !NTP_response; ntp_retry++) {
        Serial.print(F("NTP sync attempt #"));
        Serial.println(ntp_retry + 1);
        setSyncProvider(NTP_ready);
        delay(5 * Universal_1_sec_Interval);
      }
      if (!NTP_response) {
        Serial.println(F("NTP sync failed after 5 attempts, continuing without time"));
      }
    } else {
      Serial.println(F("Wifi nor connected no NTP possible"));
    }

    // MQTT connect
    Serial.println(F("Time Sync, Connecting to mqtt sevrer"));
    mqttConnect(clientId, NodeID.c_str());
    Serial.println(F("Mqtt Connection Done!, sending Device Data"));
    initManagedDevice();

    // Print startup summary
    Serial.println(F("Inicializacion boton con identificacion RFID Exitoso;"));
    Serial.println(F("Parametros de ambiente de funcionamiento:"));
    Serial.print(F("            CHIPID: "));       Serial.println(NodeID);
    Serial.print(F("            HARDWARE: "));      Serial.println(HardwareVersion);
    Serial.print(F("            FIRMWARE: "));      Serial.println(FirmwareVersion);
    Serial.print(F("            Servidor de NTP: ")); Serial.println(btnconfig.NTPClient_SERVER);
    Serial.print(F("            Servidor de MQTT: ")); Serial.println(btnconfig.MQTT_Server);
    Serial.print(F("            Puerto del Servidor de MQTT: ")); Serial.println(atoi(btnconfig.MQTT_Port));
    Serial.print(F("            Usuario de MQTT: ")); Serial.println(btnconfig.MQTT_User);
    Serial.print(F("            Client ID: "));     Serial.println(clientId);
    delay(Universal_1_sec_Interval);

    Blanco.COff();
    Last_Normal_Reset_Millis = millis();
    Last_Update_Millis = millis();
    Last_NTP_Update_Millis = millis();
    RetardoLectura = millis();
    fsm_state = STATE_IDLE;
    yield();
  }
}

// ============================================================ LOOP
void loop() {
  switch (fsm_state) {
    case STATE_IDLE:
      readTag();
      if (fsm_state != STATE_IDLE) break;
      readBtn();
      if (fsm_state != STATE_IDLE) break;
      NormalReset();
      checkalarms();

      if (millis() - Last_Update_Millis > (unsigned long)heartbeat_minutes * 60 * Universal_1_sec_Interval) {
        Last_Update_Millis = millis();
        fsm_state = STATE_UPDATE;
        break;
      }
      if (millis() - Last_NTP_Update_Millis > 60 * 60 * Universal_1_sec_Interval) {
        Last_NTP_Update_Millis = millis();
        fsm_state = STATE_UPDATE_TIME;
        break;
      }
      if (millis() - RetardoLectura > 5 * Universal_1_sec_Interval) {
        OldTagRead = "1";
        RetardoLectura = millis();
      }
      if (failed >= fail_threshold) {
        failed = 0; published = 0; sent = 0;
        ESP.restart();
      }
      if (!client.connected()) {
        mqttReconnect(clientId, NodeID.c_str());
      }
      client.loop();
      yield();
      break;

    case STATE_TRANSMIT_BOTON_DATA:
      Serial.println(F("BOTON DATA SENT"));
      if (!client.connected()) { mqttReconnect(clientId, NodeID.c_str()); }
      CheckTime();
      publish_Boton_Data();
      fsm_state = STATE_IDLE;
      break;

    case STATE_TRANSMIT_CARD_DATA:
      Serial.println(F("CARD DATA SENT"));
      if (!client.connected()) { mqttReconnect(clientId, NodeID.c_str()); }
      CheckTime();
      publishRF_ID_Lectura();
      clearBufferArray();
      fsm_state = STATE_IDLE;
      break;

    case STATE_UPDATE:
      Serial.println(F("STATE_UPDATE"));
      updateDeviceInfo();
      fsm_state = STATE_TRANSMIT_DEVICE_UPDATE;
      break;

    case STATE_TRANSMIT_DEVICE_UPDATE:
      Serial.println(F("STATE_TRANSMIT_DEVICE_UPDATE"));
      if (!client.connected()) { mqttReconnect(clientId, NodeID.c_str()); }
      CheckTime();
      publishRF_ID_Manejo();
      fsm_state = STATE_IDLE;
      break;

    case STATE_UPDATE_TIME:
      Serial.println(F("NTP_CLIENT"));
      pTimeClient->update();
      {
        int ntpRetry = 0;
        while (!NTP_response && ntpRetry < 5) {
          setSyncProvider(NTP_ready);
          delay(Universal_1_sec_Interval);
          ntpRetry++;
        }
        if (!NTP_response) {
          Serial.println(F("NTP sync failed after 5 attempts, continuing..."));
        }
      }
      NTP_response = false;
      fsm_state = STATE_IDLE;
      break;

    case STATE_RDY_TO_UPDATE_OTA:
      ArduinoOTA.handle();
      break;

    default:
      Serial.println(F("FSM: unknown state, resetting to IDLE"));
      fsm_state = STATE_IDLE;
      break;
  }

  feedback_update();
  ESP.wdtFeed();
  if (fsm_state != STATE_RDY_TO_UPDATE_OTA) {
    yield();
  }
}
