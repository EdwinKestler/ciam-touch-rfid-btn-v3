/*
  mqtt_manager.h - MQTT connection, reconnection, callback dispatch, topic subscriptions.
*/
#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

// MQTT client (accessible for publish calls from main)
extern PubSubClient client;
extern WiFiClient wifiClient;

// Counters
extern int failed, sent, published;
extern unsigned long msg_seq;

// Network info strings (updated by WiFi connect routines)
extern String Smacaddrs;
extern String Sipaddrs;

// MQTT backoff
extern unsigned long mqtt_backoff_ms;
#define MQTT_BACKOFF_MAX 60000UL

// LWT payload
extern const char LWT_PAYLOAD[];

// Initial MQTT connection (setup-time, 4 retries then fallback WiFi / portal)
void mqttConnect(const char* clientId, const char* nodeId);

// Reconnect during runtime (3 retries with exponential backoff)
void mqttReconnect(const char* clientId, const char* nodeId);

// Subscribe to all managed topics and publish device metadata
void initManagedDevice();

#endif // MQTT_MANAGER_H
