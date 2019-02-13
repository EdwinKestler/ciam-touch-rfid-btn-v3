/*
  Settings.h - Library for Operantion enviroment Variables.
  Created by Edwin Kestler, Jan 29 , 2019.
  Released into the public domain.
*/
//--------------------------------------------------------------------------------------------------//Parametros de Credenciales para OTA
const char* ssid_OTA = "RFID_OTA";
const char* password_OTA = "FLATB0X_OTA";
//--------------------------------------------------------------------------------------------------//Parametros de crecenciales para conexion a servicio de MQTT
#define ORG "FLATBOX"
#define DEVICE_TYPE "AC_WIFI_RFID_BTN"
#define DEVICE_ID "CIAM"
#define TOKEN "hY5OOupZk*U1yMl1G8"
//--------------------------------------------------------------------------------------------------// Parametros de configuracion de Hora
const int timeZone = -6;                                                                            // Eastern central Time (USA)

//--------------------------------------------------------------------------------------------------//Parametros de Version de Cambios
String FirmwareVersion = "V5.00";                                                                   //read in chage history
String HardwareVersion = "V3.00";                                                                   //read in chage history
//--------------------------------------------------------------------------------------------------//Topicos para transmision de datos a la nube.
char  publishTopic[26]  =   "iot-2/evt/status/fmt/json";
char  responseTopic[26] =   "iotdm-1/response/";
char  manageTopic[26]   =   "iotdevice-1/mgmt/manage";
char  updateTopic[26]   =   "iotdm-1/device/update";
char  rebootTopic[40]   =   "iotdm-1/mgmt/initiate/device/reboot";

//--------------------------------------------------------------------------------------------------//Parametros de Tiempos de espera
unsigned long Universal_1_sec_Interval  = 1000UL;                                                   //Variable configurable remotamente sobre el interbalo de publicacion
unsigned long Btn_conf_Mode_Interval = 2000UL;                                                      //variable configurable remotamente sobre el intervalo de espera para modo de funcionamiento de boton.

//-------- Variables de ERROR EN ENVIO de paquetes de MQTT ANTES DE REINICIO
#define FAILTRESHOLD 150
const float BATTRESHHOLD = 3.3;


