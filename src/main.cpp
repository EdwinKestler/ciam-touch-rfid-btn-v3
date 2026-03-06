/*
  main.cpp - CIAM Touch RFID Button platform
  FSM, setup, loop, and sensor read/publish functions.
  All infrastructure (config, MQTT, WiFi, feedback, OTA) is in separate modules.
  Sensors use the SensorBase abstraction for plug-and-play extensibility.
*/
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <Flatbox_Publish.h>
#include <Settings.h>
#include <ntp_sync.h>

#include <pins.h>
#include <config.h>
#include <feedback.h>
#include <ota.h>
#include <wifi_setup.h>
#include <mqtt_manager.h>

#include <sensor_base.h>
#include <rfid_sensor.h>
#include <button_sensor.h>
#include <power_sensor.h>

// --- FSM states ---
enum FsmState {
  STATE_IDLE                   = 0,
  STATE_TRANSMIT_SENSOR_DATA   = 1,
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

// --- ESP8266 identity ---
String NodeID = String(ESP.getChipId());
char clientId[50];

// --- Flatbox JSON publisher ---
flatbox Flatbox_Json(NodeID);

// --- Sensors ---
RFIDSensor rfidSensor(PIN_RFID_TX, PIN_RFID_RX, NodeID, Flatbox_Json);
ButtonSensor buttonSensor(PIN_TOUCH, NodeID, Flatbox_Json);
PowerSensor powerSensor(PIN_BATTERY, NodeID, Flatbox_Json, 30UL * 60 * 1000);

SensorBase* sensors[] = { &rfidSensor, &buttonSensor, &powerSensor };
const int SENSOR_COUNT = sizeof(sensors) / sizeof(sensors[0]);
SensorBase* activeSensor = nullptr;

// --- Timers and status ---
unsigned long Last_Normal_Reset_Millis;
unsigned long Last_Update_Millis;
unsigned long Last_NTP_Update_Millis;
char msg[20] = "";
int BeepSignalWarning = 0;
int hora = 0;
int WifiSignal;
String bootReason;

// ============================================================ Publish functions
void publishRF_ID_Manejo() {
  sent++;
  msg_seq++;
  HeartbeatData hb = {
    .status      = msg,
    .battery     = powerSensor.voltage(),
    .rssi        = WifiSignal,
    .published   = published,
    .sent        = sent,
    .failed      = failed,
    .timestamp   = ISO8601,
    .mac         = Smacaddrs.c_str(),
    .ip          = Sipaddrs.c_str(),
    .deviceId    = btnconfig.Device_ID,
    .fwVersion   = FirmwareVersion,
    .hwVersion   = HardwareVersion,
    .hora        = hora,
    .uptimeSec   = millis() / 1000,
    .freeHeap    = ESP.getFreeHeap(),
    .ssid        = WiFi.SSID().c_str(),
    .location    = btnconfig.Location,
    .seq         = msg_seq,
    .bootReason  = bootReason.c_str()
  };
  char* payload = Flatbox_Json.Administracion_Dispositivo(hb);
  if (client.publish(manageTopic, payload, true)) {
    Serial.println(F("enviado data de dispositivo:OK"));
    published++;
    failed = failed / 2;
  } else {
    Serial.print(F("enviado data de dispositivo:FAILED"));
    failed++;
  }
}

void publishSensorEvent(SensorBase* sensor) {
  sent++;
  msg_seq++;
  const char* payload = sensor->buildPayload(ISO8601, msg_seq);
  if (client.publish(sensor->topicName(), payload)) {
    sensor->onPublishOk();
    published++;
    failed = failed / 2;
  } else {
    sensor->onPublishFail();
    failed++;
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
      ntp_check_time();
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

  // Initialize sensors
  for (int i = 0; i < SENSOR_COUNT; i++) {
    sensors[i]->begin();
  }

  // Load config from LittleFS
  readConfigFromLittleFS();
  if (shouldSaveConfig) {
    saveConfigToLittleFS();
    shouldSaveConfig = false;
  }

  // Build MQTT client ID from config
  snprintf(clientId, sizeof(clientId), "d:%s:%s:%s", ORG, DEVICE_TYPE, btnconfig.Device_ID);

  // Initialize NTP client
  ntp_init(btnconfig.NTPClient_SERVER, atoi(btnconfig.NTPClient_interval));

  // --- Boot mode selection: button held during startup windows ---
  Serial.print(F("            estado del Boton: "));
  Serial.println(buttonSensor.button().check());
  delay(Universal_1_sec_Interval);

  // Window 1: Hold button for on-demand WiFi config portal
  Btn_check_Current_millis = millis();
  Verde.This_RGB_State(HIGH);
  while (millis() < (Btn_check_Current_millis + Btn_conf_Mode_Interval) && !OTA_ENABLED) {
    if (buttonSensor.button().check()) {
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
    if (buttonSensor.button().check()) {
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
    if (WiFi.status() == WL_CONNECTED) {
      ntp_sync_startup();
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
    fsm_state = STATE_IDLE;
    yield();
  }
}

// ============================================================ LOOP
void loop() {
  switch (fsm_state) {
    case STATE_IDLE:
      // Poll all sensors
      for (int i = 0; i < SENSOR_COUNT; i++) {
        sensors[i]->poll();
        if (sensors[i]->hasEvent()) {
          activeSensor = sensors[i];
          fsm_state = STATE_TRANSMIT_SENSOR_DATA;
          break;
        }
      }
      if (fsm_state != STATE_IDLE) break;

      NormalReset();
      checkalarms();
      rfidSensor.resetStaleTag();

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

    case STATE_TRANSMIT_SENSOR_DATA:
      Serial.println(F("SENSOR DATA SENT"));
      if (!client.connected()) { mqttReconnect(clientId, NodeID.c_str()); }
      ntp_check_time();
      publishSensorEvent(activeSensor);
      activeSensor = nullptr;
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
      ntp_check_time();
      publishRF_ID_Manejo();
      fsm_state = STATE_IDLE;
      break;

    case STATE_UPDATE_TIME:
      ntp_resync();
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
