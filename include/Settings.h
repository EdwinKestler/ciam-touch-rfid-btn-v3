/*
  Settings.h - Library for Operantion enviroment Variables.
  Created by Edwin Kestler, Jan 29 , 2019.
  Released into the public domain.
*/
#ifndef SETTINGS_H
#define SETTINGS_H
//--------------------------------------------------------------------------------------------------//Parametros de Credenciales para OTA
const char* ssid_OTA = "RFID_OTA";
const char* password_OTA = "FLATB0X_OTA";
//--------------------------------------------------------------------------------------------------//Parametros de crecenciales para conexion a servicio de MQTT
#define ORG "FLATBOX"
#define DEVICE_TYPE "AC_WIFI_RFID_BTN"
//--------------------------------------------------------------------------------------------------// Parametros de configuracion de Hora
const int timeZone = -6;                                                                            // Eastern central Time (USA)

//--------------------------------------------------------------------------------------------------//Parametros de Version de Cambios
const char FirmwareVersion[] = "V5.00";                                                               //read in chage history
const char HardwareVersion[] = "V3.00";                                                               //read in chage history
//--------------------------------------------------------------------------------------------------//Topicos para transmision de datos a la nube.
char  publishTopic[]  =   "iot-2/evt/status/fmt/json";
char  responseTopic[] =   "iotdm-1/response/";
char  manageTopic[]   =   "iotdevice-1/mgmt/manage";
char  updateTopic[]   =   "iotdm-1/device/update";
char  rebootTopic[]   =   "iotdm-1/mgmt/initiate/device/reboot";
char  rgbTopic[]      =   "iotdm-1/device/ctrl";

//--------------------------------------------------------------------------------------------------//Parametros de Tiempos de espera
unsigned long Universal_1_sec_Interval  = 1000UL;                                                   //Variable configurable remotamente sobre el interbalo de publicacion
unsigned long Btn_conf_Mode_Interval = 2000UL;                                                      //variable configurable remotamente sobre el intervalo de espera para modo de funcionamiento de boton.
int heartbeat_minutes = 30;                                                                          //intervalo de heartbeat en minutos, configurable remotamente

//-------- Umbral de señal WiFi baja (dBm) para alarmas y reportes
int rssi_low_threshold = -75;

//-------- Variables de ERROR EN ENVIO de paquetes de MQTT ANTES DE REINICIO
int fail_threshold = 150;

#endif // SETTINGS_H
