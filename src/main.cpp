//File Sistema for Json Configuration File
#include <LittleFS.h>                                                                                //Libreria de Sistema de Archivos (LittleFS) para el almacenamiento de variables de entorno
#include <Arduino.h>                                                                                //Librerias base de ARDUINO
//Physical Board Pheripherials
#include <Settings.h>                                                                               //Libreria que guarda los parametros del ambiente de funcionamiento
#include <BlinkRGB.h>                                                                               //Libreria para modulo de lampara RGB 5050 
#include <BTN_Bzzr.h>                                                                               //Libreria de bocina de alarmas audubles
#include <TouchPadButton.h>                                                                         //Libreria de Boton por medio de sensor capacitivo
#include <SoftwareSerial.h>                                                                          //Puerto SErial por Software
//NTP Service
#include <NTP_Client.h>                                                                              //Libreria de Syncronizacion con servicios de Hora en la RED (NTP)
#include <ESP8266WiFi.h>                                                                            //Libreria Core de Arduino para Comunicaicon Wifi del chip ESP8266
#include <WiFiUdp.h>                                                                                //Libreria de UDP necesaria para la sincornizacion con el servidor de NTP
#include <TimeLibEsp.h>                                                                             //Libreria para llevar el control de la fecha y la hora despues de sincronizar el NTP
//Arduino JSON
#include <ArduinoJson.h>                                                                            //Libreria de Arduino para el manejo de JSON
//WifiManager
#include <WiFiManager.h>                                                                            //Libreria de Wifi Mananger para configuracion de parametros de entorno SSID, PSK PASS, MQTT, NTP, NTP interval
#include <DNSServer.h>                                                                              //libreiria usada por el Wifi Manager para resolucoin de nombres.
#include <ArduinoOTA.h>                                                                             //Libreria usada para poder actulizar el codigo por medio de internet.
#include <ESP8266WebServer.h>                                                                       //libreria que levanta la pagina web en el modo de AP para configuracion de parametros de SSID, PSK PASS,MQTT, NTP intervalo
#include <ESP8266mDNS.h>                                                                            //Libreria que sirver de servidor DNS para cuando se levanta en modo de OTA.
//PublishSubscribe MQTT
#include <PubSubClient.h>                                                                           //https://github.com/knolleary/pubsubclient/releases/tag/v2.3
//Flatbox publish Service
#include <Flatbox_Publish.h>                                                                        //Libreria de manejo de Jason para Flatbox

//**********************************************************************************FIN DE DEFINICION DE LIBRERIAS EXTERNAS
//Estructura de infromacion de ambiente de configuracion
struct BtnConf {
    char  MQTT_Server [30];                                                                          //Variable de Direccion de servidor de MQTT
    char  MQTT_Port [6] = "1883";                                                                    //Variable de puerto servidor de MQTT
    char  MQTT_User[24];                                                                             //Variable de usuario para conexion con servidor de MQTT
    char  MQTT_Password[24];                                                                         //Variable de Contraseña para conexion con servidor de MQTT
    char NTPClient_SERVER[24] = "time-a-g.nist.gov";                                                //Variable de Direccion de Servidor de NTP
    char  NTPClient_interval [6];                                                                    //Variable de intervalo de sincronizacion de hora por medio de NTP
    char  Device_ID[16] = "CIAM";                                                                    //Variable de identificacion de despliegue/sitio
    char  Location[48] = "";                                                                         //Variable de ubicacion fisica del dispositivo (ej: "Edificio A, Piso 2")
    char  Wifi_Fallback_SSID[33] = "";                                                               //SSID de respaldo si la red primaria falla
    char  Wifi_Fallback_Pass[64] = "";                                                               //Password de la red WiFi de respaldo
};

BtnConf btnconfig;                                                                                  //Objeto de estructura de configuracion
//--------------------------------------------------------------------------------------------------//OTA && WifiManager
bool OTA_ENABLED = false;
bool Wifi_On_Demand_ENABLED = false;
unsigned long Btn_check_Current_millis;
//--------------------------------------------------------------------------------------------------incializacion del software Serial
SoftwareSerial RFIDReader(D2, D3, false);                                                             //se inciializa el puerto Serial por Software en los puerstos D2 como TX y D3 como RX
unsigned long RetardoLectura;                                                                       //Variable que almacena el tiempo transcurrido desde la ultima vez que se leyo una tarjeta 
long LecturaTreshold = 5000;                                                                        //Variable que almacena el tiempo que debe transcurrir en millisegundos para leer otra tarjeta
//--------------------------------------------------------------------------------------------------Variables Propias del CORE ESP8266 Para la administracion del Modulo
String NodeID = String(ESP.getChipId());                                                            //Variable Global que contiene la identidad del nodo (ChipID) o numero unico
//--------------------------------------------------------------------------------------------------definicion de parametros del Wifi
char clientId[50];                                                                                    //Variable de Identificacion de Cliente para servicio de MQTT, built at runtime from config
String  Smacaddrs = "00:00:00:00:00:00";                                                            //Variable apara almacenar la MAC Address asignada por el servicio de Wifi
String  Sipaddrs  = "000.000.000.000";                                                              //Variable para almacenar la direccion IP asignada por el servicio de wifi
//--------------------------------------------------------------------------------------------------definicon de variables para NTP
WiFiUDP ntpUDP;                                                                                     //Cliente de Servicio UDP
boolean NTP_response = false;                                                                       //Bandera de comprobacion de lectura exitosa del servicio de Hora por internet 
char ISO8601[21] = "";                                                                               //Variable para almacenar la marca del timepo (timestamp) de acuerdo al formtao ISO8601
NTPClient* pTimeClient = nullptr;                                                                    //Puntero al cliente NTP, se inicializa en setup() despues de cargar config

//--------------------------------------------------------------------------------------------------definicion de pines de Bzzr
BTN_Bzzr alarm(D5);                                                                                 //definicio de variable, pueto fisico en el board y tiempo de sonido para la bocina piezo electrica 
//defincion de tonos en millisegundos 
unsigned long tono_corto = 250;                                                                     //definicion de tono coroto de 250 milisegundos 
unsigned long tono_medio = 500;                                                                     //definicion de tono medio largo con 500 milisegundos
unsigned long tono_largo = 1000;                                                                    //definicion de tono largo con 1000 milisegundos
//--------------------------------------------------------------------------------------------------definicion de pines de RGB
BlinkRGB Azul (D6);                                                                                 //definicion del puerto fisico en el board para el led de color Azul
BlinkRGB Verde (D7);                                                                                //definicion del puerto fisico en el board para el led de color Verde
BlinkRGB Rojo (D8);                                                                                 //definicion del puerto fisico en el board para el led de color Rojo
//--------------------------------------------------------------------------------------------------//definicion de Topicos de acuerdo a publicacion
flatbox Flatbox_Json(NodeID);  // single instance — each method uses its own internal buffer
//--------------------------------------------------------------------------------------------------definicion de pines de Boton Touch
TouchPadButton T_button(D0);                                                                        //definicion del puerto fisico en el board para el boton capacitivo
int pressed_count = 0;                                                                              //definicion de variable que almacena las veces que ha sido presionado el boton
//--------------------------------------------------------------------------------------------------definicion de pines para luz del flash
BlinkColor Blanco  (D6,D7,D8);                                                                      //definicion de los puertos fisicos en el board para formar el led de color Blanco
BlinkColor Purpura (D6,D4,D8);                                                                      //definicion de los puertos fisicos en el board para formar el led de color Purpura
unsigned long flash_corto = 250;                                                                    //definicion de flash coroto de 250 milisegundos 
unsigned long flash_medio = 500;                                                                    //definicion de flash medio largo con 500 milisegundos
unsigned long flash_largo = 1000;                                                                   //definicion de flash largo con 1000 milisegundos
//--------------------------------------------------------------------------------------------------flag for saving data
bool shouldSaveConfig = false;
//--------------------------------------------------------------------------------------------------//MQTT_pubsubclient variables
int failed, sent, published;                                                                        //Variables de conteo de mensajes enviados, fallidos y publicados
unsigned long msg_seq = 0;                                                                           //Numero de secuencia monotonica para todos los mensajes MQTT
//--------------------------------------------------------------------------------------------------//MQTT reconnect backoff
unsigned long mqtt_backoff_ms = 3000;                                                                //Backoff inicial para reconexion MQTT (3s)
#define MQTT_BACKOFF_MAX 60000UL                                                                     //Backoff maximo (60s)
//--------------------------------------------------------------------------------------------------//RFID rate limiting
unsigned long lastCardPublishMillis = 0;                                                             //Ultima vez que se publico una tarjeta
String lastPublishedCardID = "";                                                                     //Ultima tarjeta publicada
#define RFID_RATE_LIMIT_MS 60000UL                                                                   //Minimo 60s entre publicaciones de la misma tarjeta
//--------------------------------------------------------------------------------------------------//Config version
#define CONFIG_VERSION 3                                                                             //Version del esquema de config.json (v3 = hex-encoded XOR password)
//--------------------------------------------------------------------------------------------------//LWT (Last Will and Testament) payload
static const char LWT_PAYLOAD[] = "{\"d\":{\"Ddata\":{\"Msg\":\"offline\"}}}";
//--------------------------------------------------------------------------------------------------//Boot reason
String bootReason;
//--------------------------------------------------------------------------------------------------//variables Globales de lectura de codigos RFID
#define DataLenght 10
#define RFID_FRAME_SIZE 13
#define TypesofCards 13

byte incomingdata;
String inputString;
char SearchSValue;
byte tagID[RFID_FRAME_SIZE];
String OldTagRead = "1";                                                                            //VAriable para guardar la ultima tag leida y evitar lecturas consecutivas
char charBuff[DataLenght];
boolean readedTag = false;
unsigned int count = 0;
char msg[20] = "";
//--------------------------------------------------------------------------------------------------//variables globales de eventos de lectura de tarjetas
int Numero_ID_Eventos_Tarjeta = 0;
//--------------------------------------------------------------------------------------------------//variables Globales de lectura de eventos de boton
char identificador_ID_Evento_Boton[32];
int Numero_ID_Evento_Boton = 0;
//--------------------------------------------------------------------------------------------------//variables Globales para reinicio de hardware cada 24 horas (pendiente de cambio por time EPOCH)
unsigned long Last_Normal_Reset_Millis;                                                             //Variable para llevar conteo del tiempo desde la ultima publicacion
unsigned long Last_Update_Millis;                                                                   //Variable para llevar conteo del tiempo desde la ultima publicacion
unsigned long Last_NTP_Update_Millis;                                                               //Variable para llevar conteo del tiempo desde la ultima sincronizacion NTP
unsigned long Last_Warning;                                                                         //Variable para llevar conteo del tiempo desde la ultima publicacion
//--------------------------------------------------------------------------------------------------//Variables Globales de alerta
int BeepSignalWarning = 0;
float VBat = 0;
int hora= 0;
//--------------------------------------------------------------------------------------------------//variables Globales de Core de Wifi para envio de estado de boton

int WifiSignal;
String Core_Version = ESP.getCoreVersion();
String SDK_version = ESP.getSdkVersion();
unsigned int CPU_Freq = ESP.getCpuFreqMHz();
unsigned int Sketch_Size = ESP.getSketchSize();

//***************************************************************************************FINITE_STATE_MACHINE_STATES:
int fsm_state;

//--------------------------------------------------------------------------------------------------Finite State Machine States
#define STATE_IDLE                    0
#define STATE_TRANSMIT_BOTON_DATA     1
#define STATE_TRANSMIT_CARD_DATA      2
#define STATE_UPDATE                  3
#define STATE_TRANSMIT_DEVICE_UPDATE  5
#define STATE_UPDATE_TIME             6
#define STATE_RDY_TO_UPDATE_OTA       7

//***************************************************************************************FIN DE DEFINICION DE VARIABLES GLOBALES
//--------------------------------------------------------------------------------------------------//Obfuscacion XOR simple para credenciales en LittleFS (no es cifrado, solo dificulta lectura casual del flash)
// XOR + hex-encode so the result is JSON-safe ASCII (no raw binary in JSON strings)
#define XOR_KEY 0xA5
static const char HEX_CHARS[] = "0123456789abcdef";

// Encode: plaintext → XOR each byte → hex string (output must be at least 2*strlen(input)+1)
void xorObfuscateHex(const char* input, char* output, size_t outLen) {
  size_t j = 0;
  for (size_t i = 0; input[i] != '\0' && j + 2 < outLen; i++) {
    uint8_t b = (uint8_t)input[i] ^ XOR_KEY;
    output[j++] = HEX_CHARS[b >> 4];
    output[j++] = HEX_CHARS[b & 0x0F];
  }
  output[j] = '\0';
}

// Decode: hex string → unhex each pair → XOR back → plaintext
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
//--------------------------------------------------------------------------------------------------//Forward declarations
void initManagedDevice();
void saveConfigToLittleFS();
//--------------------------------------------------------------------------------------------------callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Deberia Guardar la configuracion");
  shouldSaveConfig = true;
}

//--------------------------------------------------------------------------------------------------definicion de funcion que verifica y syncroniza la hora local con la hora del servicio de internet
time_t NTP_ready(){
  Serial.println(F("Transmit NTP Request"));                                                        //Mensaje Serial para verificacion del llamado de funcion en caso de falla
  pTimeClient->update();                                                                              //Funcion de la Libreria Externa que se llama para actulizar la lectura con el servicios de hora de internet
  uint32_t beginWait = millis();                                                                    //Variable para almacenar el momento en el que se empieza la espera de respuesta del servidor de hora de internet
  while (millis() - beginWait < 1500 ) {                                                            //Funcion en la que se define un periodo maximo de espera para la sincornizacion de hora de intarnet

    Serial.print(".");                                                                              //Mensaje Serial para verificacion de la lectura de tiempo UNIX del servicio de hora de internet
    delay(100);
    unsigned long EpochTime = pTimeClient->getRawTime();                                            //Alamcenamiento de la hora UNIX de forma local

    if( EpochTime >= 3610){                                                                         //condicional comparativo para confirmar Si la hora contiene un formato mas grande  cualquier cosa random
      Serial.println();
      Serial.println(F("Receive NTP Response"));                                                    //Mensaje Serial para la verificacion de ejecucion del la condicional compartiva
      NTP_response = true;                                                                          //Actualizacion de la bandera de ejecucion de syncronizacion con hora de internet
      return EpochTime + (timeZone * SECS_PER_HOUR);                                                //Calculo de la hora tomando en cuenta la zona horaria 
    }
  }
  Serial.println();
  Serial.println(F("No NTP Response :-("));                                                         //Mensaje Serial de control en caso de falla, que el tiempo transcurrido para respuesta desde el servicio de hora de internet sea mayor al de tolerancia de respuesta
  NTP_response = false;                                                                             //Actulizacion de bandera de lectura exita de Servicio de hora de internet a falso
  return 0;                                                                                         //Salida del programa sin retorno de la hora
}
//--------------------------------------------------------------------------------------------------definicion de Funcion que incia y escribe el JSON con los parametros de Configuracion de ambiente de operacion
void Read_Configuration_JSON(){
  Serial.println(F("Montando el Sistema de Archivos o FS"));                                        //Mensaje Serial de control para verificar que la rutina ha iniciado
  if (LittleFS.begin()){                                                                              //Condicional que verifica si se puede iniciar la libreria de Sistema de Archivos ó FS
    Serial.println(F("Sistema de Archivos montado"));                                               //Mensaje Serial de control para idicar que el sistema ha sido iniciado con exito
    if(LittleFS.exists("/config.json")){                                                              //Condicional que verifica si en el sistema de Arachivos se encuentra creado el archivo de configuracion
      Serial.println(F("Leyendo el Archivo de Configuracion"));                                     //Mensaje Serial de control para idicar que el archivo de configuracionh ha sido encontrado con exito
      File configFile = LittleFS.open("/config.json","r");                                            //Se Abre el archivo de configuracion
      if(configFile){                                                                               //condiconal que verifica si el archivo de configuracion pudo ser abierto
        Serial.println(F("Archivo de configuracion Abierto"));                                      //Mensaje Serial de control para idicar que el archivo de configuracionh ha sido abierto con exito
        size_t size = configFile.size();                                                            //averiguar cuanto mide el archivo y asinar ese paramtro a una variable
        std::unique_ptr<char[]>buf(new char [size]);                                                //Crear un Buffer para almacenar el contenido del archivo
        configFile.readBytes(buf.get(), size);                                                      //Leer el archivo y copiar el buiffer

        JsonDocument doc;                                                                           //Documento JSON para almacenar la informacion
        DeserializationError error = deserializeJson(doc, buf.get());                               //Deserializamos el buffer del archivo al documento JSON
        if (error){                                                                                 //Condicional de lectura del buffer
          Serial.println(F("Fallo en leer el archivo"));                                            //Mensaje Serial de control para idicar que el archivo de configuracionh NO ha sido abierto con exito
          return;
        }
        serializeJson(doc, Serial);                                                                       //Imprimir el archivo de configuracion leido al puerto Serial para verificacion
        Serial.println(F("\nparsed json"));                                                         //Mensje serial para indicar que se empieza el parseo del archivo al Json
        strlcpy(btnconfig.MQTT_Server,                                                              // Copiar a <- destino
                doc["MQTT_Server"] | "172.18.98.142" ,                                                                 // desde el valor <- Archivo
                sizeof( btnconfig.MQTT_Server) );                                                   // <- Capacidad del destino
        strlcpy(btnconfig.MQTT_Port, doc["MQTT_Port"] | "1883", sizeof(btnconfig.MQTT_Port));       //Parametro de Configuracion del puerto del servidor de MQTT
        strlcpy(btnconfig.MQTT_User, doc["MQTT_User"] | "esp8266", sizeof(btnconfig.MQTT_User));       //Parametro de Configuracion del Usuario del Servidor de MQTT
        strlcpy(btnconfig.MQTT_Password, doc["MQTT_Password"] | "esp8266" , sizeof(btnconfig.MQTT_Password));   //Parametro de Configuracion del Password del Servidor de MQTT
        strlcpy(btnconfig.NTPClient_SERVER, doc["NTPClient_SERVER"] | "time-a-g.nist.gov",
                sizeof(btnconfig.NTPClient_SERVER));                                                //Parametro de Configuracion del Servidor de NTP
        strlcpy(btnconfig.NTPClient_interval, doc["NTPClient_interval"] | "60000", sizeof(btnconfig.NTPClient_interval));                                  //Parametro de Configuracion del Intervalo de actulizacion de hora por medio de consulta al Servicio de NTP
        strlcpy(btnconfig.Device_ID, doc["Device_ID"] | "CIAM", sizeof(btnconfig.Device_ID));         //Parametro de Identificacion de despliegue/sitio
        strlcpy(btnconfig.Location, doc["Location"] | "", sizeof(btnconfig.Location));                //Parametro de ubicacion fisica del dispositivo
        strlcpy(btnconfig.Wifi_Fallback_SSID, doc["Wifi_Fallback_SSID"] | "", sizeof(btnconfig.Wifi_Fallback_SSID));
        strlcpy(btnconfig.Wifi_Fallback_Pass, doc["Wifi_Fallback_Pass"] | "", sizeof(btnconfig.Wifi_Fallback_Pass));
        // Config version migration
        int stored_version = doc["config_version"] | 0;
        if (stored_version >= 3) {
          // v3+: password is hex-encoded XOR, decode it
          char hex_pass[48];
          strlcpy(hex_pass, doc["MQTT_Password"] | "", sizeof(hex_pass));
          xorDeobfuscateHex(hex_pass, btnconfig.MQTT_Password, sizeof(btnconfig.MQTT_Password));
        } else if (stored_version == 2) {
          // v2: binary XOR — corrupted by JSON round-trip, use plaintext default
          Serial.println(F("Config v2 detected: password corrupted by binary XOR, using default"));
          strlcpy(btnconfig.MQTT_Password, "esp8266", sizeof(btnconfig.MQTT_Password));
        }
        // v0/v1: password already read as plaintext above, no action needed
        if (stored_version < CONFIG_VERSION) {
          Serial.printf("Config migration: v%d -> v%d\n", stored_version, CONFIG_VERSION);
          shouldSaveConfig = true;  // trigger re-save with hex-encoded password
        }
      }
      configFile.close();
    }
  }else {
    Serial.println(F("Fallo en Abrir el Sistema de Archivos"));                                     //Mensje serial para indicar que Fallo el montar el sistema de manejo de archivos
  }
}
//--------------------------------------------------------------------------------------------------//Funcion auxiliar para copiar parametros de WiFiManager a btnconfig y guardar en LittleFS
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

  if(shouldSaveConfig){
    Serial.println(F("Guardando Cambios de Configuracion"));
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
    if(!configFile){
      Serial.println(F("Fallo al intentar abrir el archivo de configuracion para escritura"));
      return;
    }

    serializeJson(doc, configFile);
    configFile.close();
  }
}
//--------------------------------------------------------------------------------------------------//Funcion auxiliar para preparar WiFiManager con parametros custom
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
//--------------------------------------------------------------------------------------------------//Boot to on demand WifiManager
void BOOT_TO_On_Demand_Wifi_Manager(){
  WiFiManagerParameter custom_mqtt_server("server","MQTT_Server",btnconfig.MQTT_Server,64);
  WiFiManagerParameter custom_mqtt_port("port","MQTT_Port",btnconfig.MQTT_Port,6);
  WiFiManagerParameter custom_mqtt_user("user","MQTT_User",btnconfig.MQTT_User,64);
  WiFiManagerParameter custom_mqtt_pass("password","MQTT_Password",btnconfig.MQTT_Password,64);
  WiFiManagerParameter custom_NTPClient_SERVER("NTPServer","NTPClient_SERVER",btnconfig.NTPClient_SERVER,64);
  WiFiManagerParameter custom_NTPClient_interval("NTPInterval","NTPClient_interval",btnconfig.NTPClient_interval,6);
  WiFiManagerParameter custom_Device_ID("deviceid","Device_ID",btnconfig.Device_ID,16);
  WiFiManagerParameter custom_Location("location","Location",btnconfig.Location,48);
  WiFiManagerParameter custom_Fallback_SSID("fb_ssid","Fallback_SSID",btnconfig.Wifi_Fallback_SSID,33);
  WiFiManagerParameter custom_Fallback_Pass("fb_pass","Fallback_Pass",btnconfig.Wifi_Fallback_Pass,64);

  WiFiManager wifiManager;
  setupWifiManagerParams(wifiManager, custom_mqtt_server, custom_mqtt_port, custom_mqtt_user,
                         custom_mqtt_pass, custom_NTPClient_SERVER, custom_NTPClient_interval, custom_Device_ID,
                         custom_Location, custom_Fallback_SSID, custom_Fallback_Pass);

  Serial.println(F("Empezando Configuracion de WIFI Bajo Demanda"));
  Purpura.COn();
  alarm.Beep(tono_medio);
  alarm.Beep(tono_medio);
  if (!wifiManager.startConfigPortal("flatwifi")) {
    ESP.reset();
    delay(5 * Universal_1_sec_Interval);
  }

  copyWifiManagerParams(custom_mqtt_server, custom_mqtt_port, custom_mqtt_user,
                        custom_mqtt_pass, custom_NTPClient_SERVER, custom_NTPClient_interval, custom_Device_ID,
                        custom_Location, custom_Fallback_SSID, custom_Fallback_Pass);
}
//--------------------------------------------------------------------------------------------------//Boot to auto WifiManager
void BOOT_TO_Wifi_Manager(){
  WiFiManagerParameter custom_mqtt_server("server","MQTT_Server",btnconfig.MQTT_Server,64);
  WiFiManagerParameter custom_mqtt_port("port","MQTT_Port",btnconfig.MQTT_Port,6);
  WiFiManagerParameter custom_mqtt_user("user","MQTT_User",btnconfig.MQTT_User,64);
  WiFiManagerParameter custom_mqtt_pass("password","MQTT_Password",btnconfig.MQTT_Password,64);
  WiFiManagerParameter custom_NTPClient_SERVER("NTPServer","NTPClient_SERVER",btnconfig.NTPClient_SERVER,64);
  WiFiManagerParameter custom_NTPClient_interval("NTPInterval","NTPClient_interval",btnconfig.NTPClient_interval,6);
  WiFiManagerParameter custom_Device_ID("deviceid","Device_ID",btnconfig.Device_ID,16);
  WiFiManagerParameter custom_Location("location","Location",btnconfig.Location,48);
  WiFiManagerParameter custom_Fallback_SSID("fb_ssid","Fallback_SSID",btnconfig.Wifi_Fallback_SSID,33);
  WiFiManagerParameter custom_Fallback_Pass("fb_pass","Fallback_Pass",btnconfig.Wifi_Fallback_Pass,64);

  WiFiManager wifiManager;
  setupWifiManagerParams(wifiManager, custom_mqtt_server, custom_mqtt_port, custom_mqtt_user,
                         custom_mqtt_pass, custom_NTPClient_SERVER, custom_NTPClient_interval, custom_Device_ID,
                         custom_Location, custom_Fallback_SSID, custom_Fallback_Pass);

  Serial.println(F("Empezando Configuracion de WIFI en Automatico"));
  Purpura.COn();
  if (!wifiManager.autoConnect("flatwifi")) {
    alarm.Beep(tono_corto);
    alarm.Beep(tono_corto);
    Purpura.CFlash(flash_medio);
    if (!wifiManager.startConfigPortal("flatwifi")) {
      ESP.reset();
      delay(5 * Universal_1_sec_Interval);
    }
  }

  copyWifiManagerParams(custom_mqtt_server, custom_mqtt_port, custom_mqtt_user,
                        custom_mqtt_pass, custom_NTPClient_SERVER, custom_NTPClient_interval, custom_Device_ID,
                        custom_Location, custom_Fallback_SSID, custom_Fallback_Pass);
}
//--------------------------------------------------------------------------------------------------//Funcion para guardar config.json desde btnconfig (para actualizaciones remotas)
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
//--------------------------------------------------------------------------------------------------//Funcion para intentar conectar a la red WiFi de respaldo
bool tryFallbackWifi() {
  if (strlen(btnconfig.Wifi_Fallback_SSID) == 0) {
    return false;
  }
  Serial.print(F("Intentando red WiFi de respaldo: "));
  Serial.println(btnconfig.Wifi_Fallback_SSID);
  WiFi.begin(btnconfig.Wifi_Fallback_SSID, btnconfig.Wifi_Fallback_Pass);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("Conectado a red WiFi de respaldo"));
    return true;
  }
  Serial.println(F("Fallo conexion a red WiFi de respaldo"));
  return false;
}
//--------------------------------------------------------------------------------------------------//funcion donde se define el inicio a Actulizacion por OTA
void BOOT_TO_OTA() {
  Serial.println(F("Starting OTA"));
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid_OTA, password_OTA);
  ArduinoOTA.setPassword(password_OTA);
  ArduinoOTA.onStart([]() {
    Serial.println(F("OTA: update starting..."));
    Azul.This_RGB_State(HIGH);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\nOTA: update complete, rebooting"));
    Azul.This_RGB_State(LOW);
    alarm.Beep(tono_corto);
    alarm.Beep(tono_corto);
    alarm.Beep(tono_corto);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
    Rojo.Flash(flash_medio);
    alarm.Beep(tono_medio);
    alarm.Beep(tono_medio);
  });
  ArduinoOTA.begin();
  Serial.println(F("Ready"));
  Azul.Flash(flash_medio);
  fsm_state = STATE_RDY_TO_UPDATE_OTA;
  alarm.Beep(tono_medio);
  alarm.Beep(tono_corto);
  alarm.Beep(tono_medio);
}

//--------------------------------------------------------------------------------------------------//Funcion Remota para manejar Actulizacion de parametros por la via remota (MQTT SERVER)
void handleUpdate(byte* payload) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, (char*)payload);
  if (error) {
    Serial.println(F("ERROR en la Letura del JSON Entrante"));
    return;
  }
  Serial.println(F("Update payload:"));
  serializeJsonPretty(doc, Serial);
  Serial.println();

  // Apply configurable parameters if present
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

  // Remote location update
  if (doc["location"].is<const char*>()) {
    strlcpy(btnconfig.Location, doc["location"], sizeof(btnconfig.Location));
    Serial.printf("Updated location: %s\n", btnconfig.Location);
    saveConfigToLittleFS();
  }

  // Remote WiFi credential update — stores new creds, device will use them on next reconnect
  if (doc["wifi_ssid"].is<const char*>() && doc["wifi_pass"].is<const char*>()) {
    Serial.println(F("Remote WiFi credential update received"));
    // Store as fallback so device can try them if primary fails
    strlcpy(btnconfig.Wifi_Fallback_SSID, doc["wifi_ssid"], sizeof(btnconfig.Wifi_Fallback_SSID));
    strlcpy(btnconfig.Wifi_Fallback_Pass, doc["wifi_pass"], sizeof(btnconfig.Wifi_Fallback_Pass));
    saveConfigToLittleFS();
    Serial.printf("Stored new WiFi creds as fallback: %s\n", btnconfig.Wifi_Fallback_SSID);
  }

  // Remote fallback SSID update (separate from primary WiFi push)
  if (doc["fallback_ssid"].is<const char*>() && doc["fallback_pass"].is<const char*>()) {
    strlcpy(btnconfig.Wifi_Fallback_SSID, doc["fallback_ssid"], sizeof(btnconfig.Wifi_Fallback_SSID));
    strlcpy(btnconfig.Wifi_Fallback_Pass, doc["fallback_pass"], sizeof(btnconfig.Wifi_Fallback_Pass));
    saveConfigToLittleFS();
    Serial.printf("Updated fallback WiFi: %s\n", btnconfig.Wifi_Fallback_SSID);
  }

  // Remote OTA trigger
  if (doc["ota"].is<bool>() && doc["ota"].as<bool>()) {
    Serial.println(F("Remote OTA trigger received"));
    BOOT_TO_OTA();
  }
}

//--------------------------------------------------------------------------------------------------//Funcion remota para mandar a dormir el esp despues de enviar un RFID
void handleResponse (byte* payloadrsp) {
  JsonDocument doc;                                                                                 //Documento JSON para almacenar los mensajes
  DeserializationError error = deserializeJson(doc, (char*)payloadrsp);                             //Deserializamos el payload al documento JSON
  if (error) {                                                                                      //Si no se puede deserializar el Json
    Serial.println(F("ERROR en la Letura del JSON Entrante"));                                      //Se imprime un mensaje de Error en la lectura del JSON
    return;                                                                                         //Nos salimos de la funcion
  }                                                                                                 //se cierra el condicional

  Serial.println(F("Response payload:"));                                                           //si se pudo encontrar la raiz del objeto JSON se imprime u mensje
  serializeJson(doc, Serial);                                                                       //y se imprime el mensaje recibido al Serial
  Serial.println();                                                                                 //dejamos una linea de pormedio para continuar con los mensajes de debugging
}
//--------------------------------------------------------------------------------------------------//Funcion para parsear un valor hex de un caracter
static int hexCharToInt(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return 10 + c - 'a';
  if (c >= 'A' && c <= 'F') return 10 + c - 'A';
  return -1;
}
//--------------------------------------------------------------------------------------------------//Funcion para control remoto de RGB via MQTT
void handleRGBCommand(byte* payload) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, (char*)payload);
  if (error) {
    Serial.println(F("RGB: JSON parse error"));
    return;
  }

  int r = -1, g = -1, b = -1;

  // Check for buzzer command (accepts int or string)
  if (doc["beep"].is<int>() || doc["beep"].is<const char*>()) {
    int seconds = doc["beep"].is<int>() ? (int)doc["beep"] : atoi(doc["beep"].as<const char*>());
    seconds = constrain(seconds, 1, 300);
    Serial.printf("Buzzer: %d seconds\n", seconds);
    alarm.BeepNonBlocking((unsigned long)seconds * 1000UL);
    return;
  }

  // Check for on/off command
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
  // Check for hex color
  else if (doc["hex"].is<const char*>()) {
    const char* hex = doc["hex"];
    if (hex[0] == '#') hex++;  // skip leading #
    if (strlen(hex) == 6) {
      r = hexCharToInt(hex[0]) * 16 + hexCharToInt(hex[1]);
      g = hexCharToInt(hex[2]) * 16 + hexCharToInt(hex[3]);
      b = hexCharToInt(hex[4]) * 16 + hexCharToInt(hex[5]);
    } else {
      Serial.println(F("RGB: invalid hex format, expected 6 chars"));
      return;
    }
  }
  // Check for r, g, b values
  else if (doc["r"].is<int>() && doc["g"].is<int>() && doc["b"].is<int>()) {
    r = doc["r"];
    g = doc["g"];
    b = doc["b"];
  }
  else {
    Serial.println(F("RGB: payload must have {r,g,b}, {hex}, or {cmd}"));
    return;
  }

  // Apply the color
  Rojo.SetPWM(r);
  Verde.SetPWM(g);
  Azul.SetPWM(b);
  Serial.printf("RGB set to R:%d G:%d B:%d\n", r, g, b);
}
//--------------------------------------------------------------------------------------------------//Funcion de vigilancia sobre mensajeria remota desde el servicion de IBM bluemix
void callback(char* topic, byte* payload, unsigned int payloadLength) {                             //Esta Funcion vigila los mensajes que se reciben por medio de los Topicos de respuesta;
  Serial.print(F("Mensaje recibido desde el Topico: "));                                            //Imprimir un mensaje seÃ±alando sobre que topico se recibio un mensaje
  Serial.println(topic);                                                                            //Imprimir el Topico

  if (strcmp (responseTopic , topic) == 0) {                                                         //verificar si el topico conicide con el Topico responseTopic[] definido en el archivo settings.h local
    handleResponse(payload);
    //return; // just print of response for now                                                     //Hacer algo si conicide (o en este caso hacer nada)
  }

  if (strcmp (rebootTopic, topic) == 0) {                                                           //verificar si el topico conicide con el Topico rebootTopic[] definido en el archivo settings.h local
    Serial.println(F("Reiniciando..."));                                                            //imprimir mensaje de Aviso sobre reinicio remoto de unidad.
    ESP.reset();                                                                                    //Emitir comando de reinicio para ESP8266
  }

  if (strcmp (updateTopic, topic) == 0) {                                                           //verificar si el topico conicide con el Topico updateTopic[] definido en el archivo settings.h local
    handleUpdate(payload);                                                                          //enviar a la funcion handleUpdate el contenido del mensaje para su parseo.
  }

  if (strcmp (rgbTopic, topic) == 0) {                                                              //verificar si el topico conicide con el Topico rgbTopic[] definido en el archivo settings.h local
    handleRGBCommand(payload);                                                                      //enviar a la funcion handleRGBCommand el contenido del mensaje para control de RGB
  }
}
//--------------------------------------------------------------------------------------------------//definicion de Cliente WIFI para ESP8266 y cliente de publicacion y subcripcion
WiFiClient wifiClient;                                                                              //Se establece el Cliente Wifi
PubSubClient client(wifiClient);                                                                    //se establece el Cliente para el servicio MQTT
//--------------------------------------------------------------------------------------------------//funcion para conectar al servidor de MQTT
void mqttConnect() {
  //int port_parsed = String(btnconfig.MQTT_Port).toInt();
  client.setServer(btnconfig.MQTT_Server, atoi(btnconfig.MQTT_Port));
  client.setCallback(callback);
  if (!client.connected()) {
    Serial.print(F("Conectando al servidor MQTT: "));
    Serial.println(btnconfig.MQTT_Server);
    char charBuf[50];
    String CID (clientId + NodeID);
    CID.toCharArray(charBuf, 50);
    for (int retry = 0; retry < 4; retry++) {
      Serial.print(F("MQTT attempt #"));
      Serial.println(retry + 1);
      if (client.connect(charBuf, btnconfig.MQTT_User, btnconfig.MQTT_Password, manageTopic, 0, true, LWT_PAYLOAD)) {
        Serial.println(F("MQTT connected"));
        mqtt_backoff_ms = 3000;  // reset backoff on success
        return;
      }
      Serial.print(F("failed, rc="));
      Serial.println(client.state());
      Blanco.CFlash(flash_corto);
      delay(3000);
    }
    Serial.println(F("MQTT connect failed after 4 attempts"));
    // Try fallback WiFi before opening WiFiManager portal
    if (tryFallbackWifi()) {
      Serial.println(F("Fallback WiFi connected, retrying MQTT..."));
      Sipaddrs = WiFi.localIP().toString();
      Smacaddrs = String(WiFi.macAddress());
      return;  // caller will retry MQTT on new network
    }
    Serial.println(F("Opening WiFiManager..."));
    BOOT_TO_On_Demand_Wifi_Manager();
    ESP.restart();
  }
}
//----------------------------------------------------------------------Funcion de REConexion a Servicio de MQTT
void MQTTreconnect() {
  // Attempt up to 3 reconnections per call with exponential backoff
  for (int retry = 0; retry < 3 && !client.connected(); retry++) {
    Serial.print(F("Attempting MQTT connection..."));
    Blanco.CFlash(flash_corto);
    alarm.Beep(tono_corto);
    char charBuf[50];
    String CID (clientId + NodeID);
    CID.toCharArray(charBuf, 50);
    if (client.connect(charBuf, btnconfig.MQTT_User, btnconfig.MQTT_Password, manageTopic, 0, true, LWT_PAYLOAD)) {
      Serial.println(F("connected"));
      mqtt_backoff_ms = 3000;  // reset backoff on success
      initManagedDevice();     // re-subscribe topics after reconnect
      return;
    }
    Purpura.CFlash(flash_medio);
    alarm.Beep(tono_medio);
    Serial.print(F("failed, rc="));
    Serial.print(client.state());
    Serial.printf(" backoff: %lums\n", mqtt_backoff_ms);
    delay(mqtt_backoff_ms);
    // Exponential backoff: double delay, cap at max
    mqtt_backoff_ms = min(mqtt_backoff_ms * 2, MQTT_BACKOFF_MAX);
  }
  if (!client.connected()) {
    Serial.println(F("MQTT reconnect failed after 3 attempts"));
  }
}
//--------------------------------------------------------------------------------------------------//Funcion encargada de subscribir el nodo a los servicio de administracion remota y de notificar los para metros configurables al mismo
void initManagedDevice() {
  if (client.subscribe(responseTopic)) {                                                            //Subscribir el nodo al servicio de mensajeria de respuesta
    Serial.println(F("se ha subscrito al Topico de respuestas"));                                   //si se logro la sibscripcion entonces imprimir un mensaje de exito
  }
  else {
    Serial.println(F("No se pudo Subscribir al Topico de Respuestas"));                             //Si no se logra la subcripcion imprimir un mensaje de error
  }
  
  if (client.subscribe(rebootTopic)) {                                                              //Subscribir el nodo al servicio de mensajeria de reinicio remoto
    Serial.println(F("se ha subscrito al Topico de Reincio Remoto"));                               //si se logro la sibscripcion entonces imprimir un mensaje de exito
  }
  else {
    Serial.println(F("No se pudo Subscribir al Topico de Reinicio Remoto"));                        //Si no se logra la subcripcion imprimir un mensaje de error                
  }
  
  if (client.subscribe(updateTopic)) {                                                              //Subscribir el nodo al servicio de mensajeria de reinicio remoto
    Serial.println(F("se ha subscrito al Topico de Actulizaciones Remotas"));                       //si se logro la sibscripcion entonces imprimir un mensaje de exito
  }
  else {
    Serial.println(F("No se pudo Subscribir al Topico de Actulizacione Remotas"));                  //Si no se logra la subcripcion imprimir un mensaje de error
  }

  if (client.subscribe(rgbTopic)) {                                                                 //Subscribir el nodo al servicio de mensajeria de control RGB
    Serial.println(F("se ha subscrito al Topico de Control RGB"));                                   //si se logro la sibscripcion entonces imprimir un mensaje de exito
  }
  else {
    Serial.println(F("No se pudo Subscribir al Topico de Control RGB"));                             //Si no se logra la subcripcion imprimir un mensaje de error
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
  deviceInfo["IPAddress"]= Sipaddrs;
  static char buff[600];
  serializeJson(doc, buff, sizeof(buff));
  Serial.println(F("publishing device manageTopic metadata:"));
  Serial.println(buff);
  sent++;
  if (client.publish(manageTopic, buff)) {
    Serial.println(F("device Publish ok"));
    alarm.Beep(tono_corto);
    Verde.Flash(flash_corto);
    published ++;
  }else {
    Serial.println(F("device Publish failed:"));
    alarm.Beep(tono_medio);
    Rojo.Flash(flash_medio);
    failed++;
  }
}
//**********************************************************************************FIN DE FUNCIONES PREVIAS AL SETUP
//--------------------------------------------------------------------------------------------------//SETUP
void setup() {
  ESP.wdtEnable(8000);                                                                              //Watchdog timer: 8 segundos — reinicia si loop() se bloquea
  bootReason = ESP.getResetReason();                                                                //Capturar razon de reinicio antes de que se pierda
  Blanco.COff();                                                                                    //enviamos este comando para mandar todos los pines de RGB a LOW
  //inciamos los Seriales de hardware
  Serial.begin(115200);                                                                             //inciamos el puerto Serial de hardware a la velosidad indicada (def:15200)
  Serial.print(F("Boot reason: "));
  Serial.println(bootReason);
  Serial.println(F("inicio exitosamnte el puerto Serial"));                                         //Mensaje Serial para la verificacion del incio del puerto serial
  Serial.println(F(""));                                                                            //Linea de Mensaje Serial intencionalmente dejada en Blanco para facilidad de lectura de mensajes Seriales
  //inciamos los Seriales de software
  RFIDReader.begin(9600);                                                                           //inciamos el puerto Serial por Software a la velocidad indicada (def: 9600)
  //leemos los parametros de configuracion almacenados en la memoria en el JSON de configuracion
  Read_Configuration_JSON();
  // If config migration flagged shouldSaveConfig, save now with new schema
  if (shouldSaveConfig) {
    saveConfigToLittleFS();
    shouldSaveConfig = false;
  }
  //Construir clientId a partir de config cargada de LittleFS
  snprintf(clientId, sizeof(clientId), "d:%s:%s:%s", ORG, DEVICE_TYPE, btnconfig.Device_ID);
  //Inicializar el cliente NTP con la configuracion cargada de LittleFS
  pTimeClient = new NTPClient(btnconfig.NTPClient_SERVER, 0, atoi(btnconfig.NTPClient_interval));
  //Configutacion del Wifi
  //verificar si el boton esta apoachado para configurar wifi
  Serial.print(F("            estado del Boton: "));
  Serial.println(T_button.check());
  delay(Universal_1_sec_Interval);

  Btn_check_Current_millis = millis();
  Verde.This_RGB_State(HIGH);

  while(millis() < (Btn_check_Current_millis + Btn_conf_Mode_Interval ) && (OTA_ENABLED == LOW)){
    if(T_button.check()){
      BOOT_TO_On_Demand_Wifi_Manager();
      Wifi_On_Demand_ENABLED = HIGH;
    }
    delay(11);
  }

  Verde.This_RGB_State(LOW);
  Rojo.This_RGB_State(HIGH);
  Btn_check_Current_millis = millis();
  while(millis()< (Btn_check_Current_millis + Btn_conf_Mode_Interval ) && (Wifi_On_Demand_ENABLED == LOW)){
    if(T_button.check()){
      BOOT_TO_OTA();
      OTA_ENABLED = HIGH;
    }
    delay(11);
  }
  Rojo.This_RGB_State(LOW);
  //Revisar si el boton ya cuenta con configuracion de wifi si no enviar a aplicacion de configuracion.
  if (OTA_ENABLED == LOW){
    //configuracion automatica del wifi
    BOOT_TO_Wifi_Manager();
    if (WiFi.status() != WL_CONNECTED) {
      // Primary WiFi failed, try fallback before giving up
      if (!tryFallbackWifi()) {
        // Fallback also failed, keep retrying primary
        while (WiFi.status() != WL_CONNECTED) {
          BOOT_TO_Wifi_Manager();
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
    Serial.println(F(""));                                                         //dejamos una linea en blanco en la terminal 
    //inciamos la configuracion del NTP
    Serial.print(F("servidor de NTP: "));
    Serial.println(btnconfig.NTPClient_SERVER);
    Serial.print(F("Intervalo de actualizacion: "));
    int NTP_Update_interval = atoi(btnconfig.NTPClient_interval);
    Serial.println(NTP_Update_interval);
    //iniciamos el cliente de NTP
    pTimeClient->begin();

    if(WiFi.status() == WL_CONNECTED){
      for (int ntp_retry = 0; ntp_retry < 5 && !NTP_response; ntp_retry++) {
        Serial.print(F("NTP sync attempt #"));
        Serial.println(ntp_retry + 1);
        setSyncProvider(NTP_ready);
        delay(5*Universal_1_sec_Interval);
      }
      if (!NTP_response) {
        Serial.println(F("NTP sync failed after 5 attempts, continuing without time"));
      }
    }else{
      Serial.println(F("Wifi nor connected no NTP possible"));
    }
    //configuracion de MQTT
    //----------------------------------------------------------------------------------------------//Connectando a servicio de MQTT
    Serial.println(F("Time Sync, Connecting to mqtt sevrer"));
    mqttConnect();                                                                                  //Conectamos al servicio de Mqtt con las credenciales provistas en el archivo "settings.h"
    Serial.println(F("Mqtt Connection Done!, sending Device Data"));
    //----------------------------------------------------------------------------------------------//Enviando datos de primera conexion
    initManagedDevice();                                                                            //inciamos la administracion remota desde Bluemix
    //----------------------------------------------------------------------------------------------//Resumimos el estado de la configuraciondel boton.
    //Mensaje de configuracion de boton exitoso 
    Serial.println(F("Inicializacion boton con identificacion RFID Exitoso;"));
    Serial.println(F("Parametros de ambiente de funcionamiento:"));
    Serial.print(F("            CHIPID: "));
    Serial.println(NodeID);
    Serial.print(F("            HARDWARE: "));
    Serial.println(HardwareVersion);
    Serial.print(F("            FIRMWARE: "));
    Serial.println(FirmwareVersion);
    Serial.print(F("            Servidor de NTP: "));
    Serial.println(btnconfig.NTPClient_SERVER);
    Serial.print(F("            Servidor de MQTT: "));
    Serial.println(btnconfig.MQTT_Server);
    Serial.print(F("            Puerto del Servidor de MQTT: "));
    Serial.println(atoi(btnconfig.MQTT_Port));
    Serial.print(F("            Usuario de MQTT: "));
    Serial.println(btnconfig.MQTT_User);
    Serial.print(F("            Client ID: "));
    Serial.println(clientId);
    delay(Universal_1_sec_Interval);
    // put your setup code here, to run once:
    Blanco.COff();
    Last_Normal_Reset_Millis = millis();
    Last_Update_Millis = millis();
    Last_NTP_Update_Millis = millis();
    RetardoLectura = millis();
    fsm_state = STATE_IDLE; //inciar el estado del la maquina de stado finito
    yield();
  }
}
//****************************************************************************Inicio de funciones ejecutadas en el loop
//----------------------------------------------------------------------------funcion que procesa como desplegar y transmitir la hora de acuerdo al formato del ISO8601
void CheckTime(){ //digital clock display of the time
  static time_t prevDisplay = 0;
  if (timeStatus() != timeNotSet) {
    time_t t = now();
    if (t != prevDisplay) {                                                 //update the display only if time has changed
      prevDisplay = t;
      snprintf(ISO8601, sizeof(ISO8601), "%04d-%02d-%02dT%02d:%02d:%02d", year(), month(), day(), hour(), minute(), second());
    }
  }
  else {
    Serial.println(F("Time not Sync, Syncronizing time"));
    NTP_ready();
  }
}
//-----------------------------------------------------------------------------------Limpiando el Buffer donde se almacena los tarjetas
void clearBufferArray() {             // function to clear buffer array
  inputString = "";
  for (unsigned int i = 0; i < RFID_FRAME_SIZE; i++) {
    tagID[i] = 0; // clear all index of array with command NULL
  }
}
//---------------------------------------------------------------------------------------------------Leer la tarjeta que se presenta
void readTag() {
  if (RFIDReader.available()) {
    inputString = "";
    while (RFIDReader.available() > 0) {
      // If data available from reader
      incomingdata = RFIDReader.read();
      tagID[count] = incomingdata;
      if (count > 3 && count < 8) {
        inputString += incomingdata;
      }
      delay(2);  // ~1ms per byte at 9600 baud, 2ms margin is sufficient
      if (count == 12) break;
      count++ ;
    }
    count = 0;
    // Validate RFID frame: must have extracted card ID bytes (positions 4-7)
    // Also reject all-zero IDs (phantom reads when no card is present)
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
    // Reject duplicate consecutive reads early — avoid unnecessary state transition and feedback
    if (OldTagRead == inputString) {
      Serial.println(F("Duplicate read, ignoring"));
      return;
    }
    Verde.Flash(flash_corto);
    alarm.Beep(tono_corto);
    readedTag = !readedTag;
    fsm_state = STATE_TRANSMIT_CARD_DATA;
  }
  return;
}
//---------------------------------------------------------------------------------------------- fucnion de lectura de activiad del boton
void readBtn() {
  if (T_button.check() == true){
    Serial.println(F("Pressed"));
    Numero_ID_Evento_Boton ++;
    snprintf(identificador_ID_Evento_Boton, sizeof(identificador_ID_Evento_Boton), "%s-%d", NodeID.c_str(), Numero_ID_Evento_Boton);
    Azul.Flash(flash_corto);
    alarm.Beep(tono_corto);
    fsm_state = STATE_TRANSMIT_BOTON_DATA; //PUTS FSM MACHINE ON TRANSMIT DATA MODE
  }
  return;
}

//-------- Data de Manejo RF_ID_Manejo. Publish the data to MQTT server, the payload should not be bigger than 45 characters name field and data field counts. --------//
void publishRF_ID_Manejo () {
  sent++;
  msg_seq++;
  char* payload = Flatbox_Json.Administracion_Dispositivo(msg,VBat,WifiSignal,published,sent,failed,ISO8601,Smacaddrs,Sipaddrs,btnconfig.Device_ID,FirmwareVersion,HardwareVersion,hora,millis()/1000,ESP.getFreeHeap(),WiFi.SSID().c_str(),btnconfig.Location,msg_seq,bootReason.c_str());
  if (client.publish(manageTopic, payload, true)) {  // retain=true so dashboard gets last known state
    Serial.println(F("enviado data de dispositivo:OK"));
    published ++;
    failed = failed / 2;  // decay instead of reset — masks persistent degradation less
  } else {
    Serial.print(F("enviado data de dispositivo:FAILED"));
    failed ++;
  }
}

//------------------------------------------------------------------------------------------------Funcion de reseteo normal
void NormalReset() {
  if (millis() - Last_Normal_Reset_Millis > 60 * 60 * Universal_1_sec_Interval) {
    hora++;
    WifiSignal = WiFi.RSSI();
    if (hora > 24) {
      strlcpy(msg, "24h Normal Reset", sizeof(msg));
      VBat = analogRead(A0) * (4.2 / 1024.0); // TODO: calibrate voltage divider ratio for actual hardware
      if (!client.connected()) { MQTTreconnect(); }
      CheckTime();
      publishRF_ID_Manejo();
      client.disconnect();
      hora = 0;
      ESP.restart();
    }
    Last_Normal_Reset_Millis = millis(); //Actulizar la ultima hora de envio
  }
}
//--------------------------------------------------------------------------Funcion de checkear alarmas.!!!------------------------------------------------------------------------------
void checkalarms () {
  if (WiFi.RSSI() < rssi_low_threshold) {
    if (BeepSignalWarning < 4) {
      alarm.Beep(tono_largo);
      BeepSignalWarning++;
    }
    Blanco.CFlash(flash_largo);
  } else {
    BeepSignalWarning = 0;
  }
}

//--------------------------------------------------------------------------Funcion de publicar los datos de estado si ha pasado el tiempo establecido entonces*!!------------------------------------------------------------------------------
void updateDeviceInfo() {
  strlcpy(msg, "on", sizeof(msg));
  VBat = analogRead(A0) * (4.2 / 1024.0); // TODO: calibrate voltage divider ratio for actual hardware
  WifiSignal = WiFi.RSSI();
  if (WiFi.RSSI() < rssi_low_threshold) {
    strlcpy(msg, "LOWiFi", sizeof(msg));
    Rojo.Flash(flash_medio);
    alarm.Beep(tono_medio);
    Serial.print(WiFi.SSID());
    Serial.print(" ");
    Serial.println(WiFi.RSSI());
    fsm_state = STATE_TRANSMIT_DEVICE_UPDATE;
    return;
  }
}

//---------------------------------------------------------------------------funcion de enviode Datos Boton RF_Boton.-----------------------
void publish_Boton_Data(){
  sent++;
  msg_seq++;
  if (client.publish(publishTopic, Flatbox_Json.Evento_Boton(ISO8601, identificador_ID_Evento_Boton, msg_seq))) {
    Serial.println(F("enviado data de boton: OK"));
    Verde.Flash(flash_corto);
    alarm.Beep(tono_corto);
    published ++;
    failed = failed / 2;
    } else {
      Serial.println(F("enviado data de boton: FAILED"));
      Rojo.Flash(flash_corto);
      failed ++;
    }
    Blanco.COff();
}
//-------- funcion datos Lectura Tag RF_ID_LECTURA. Publish the data to MQTT server, the payload should not be bigger than 45 characters name field and data field counts. --------//

boolean publishRF_ID_Lectura() {
  if (OldTagRead != inputString) {
    // Rate limit: same card ID can only be published once per RFID_RATE_LIMIT_MS
    if (inputString == lastPublishedCardID && millis() - lastCardPublishMillis < RFID_RATE_LIMIT_MS) {
      Serial.println(F("RFID: rate limited, same card too soon"));
      OldTagRead = inputString;
      inputString = "";
      return false;
    }
    OldTagRead = inputString;
    lastPublishedCardID = inputString;
    lastCardPublishMillis = millis();
    Numero_ID_Eventos_Tarjeta ++;
    char Identificador_ID_Evento_Tarjeta[32];
    snprintf(Identificador_ID_Evento_Tarjeta, sizeof(Identificador_ID_Evento_Tarjeta), "%s-%d", NodeID.c_str(), Numero_ID_Eventos_Tarjeta);
    sent ++;
    msg_seq++;
    if (client.publish(publishTopic, Flatbox_Json.Evento_Tarjeta(Identificador_ID_Evento_Tarjeta,ISO8601,inputString,msg_seq))) {
      Serial.println(F("enviado data de RFID: OK"));
      Verde.Flash(flash_corto);
      alarm.Beep(tono_corto);
      published ++;
      inputString = "";
      failed = failed / 2;
      return true;
    } else {
      Serial.println(F("enviado data de RFID: FAILED"));
      Rojo.Flash(flash_corto);
      failed ++;
      OldTagRead = "1";
      inputString = "";
      return false;
    }
  } else {
    Serial.println("Este es una lectura consecutiva");
    return false;
  }
}

//**************************************************************************************************INICIO DE FUNCIONES DE LOOP
//--------------------------------------------------------------------------------------------------Funcion de bucle infinito (Loop) este codigo se ejecuta repetitivamente
void loop() {
  //iniciamos el Switch de estado
  switch(fsm_state){                                                                                //se lee en que stado debe iniciar el Switch de estados
    case STATE_IDLE :                                                                               //Si el estado que se lee es el por defecto IDLE cunado el boton no hace nada 90% del tiempo
      readTag();                                                                                    //leer su hay alguna tarjeta
      if (fsm_state != STATE_IDLE) break;                                                           //prioridad: card > button > timers
      readBtn();                                                                                    //leer si se presiono el boton
      if (fsm_state != STATE_IDLE) break;
      NormalReset();
      checkalarms();
      if (millis() - Last_Update_Millis > (unsigned long)heartbeat_minutes * 60 * Universal_1_sec_Interval) {
        Last_Update_Millis = millis(); //Actulizar la ultima hora de envio
        fsm_state = STATE_UPDATE;
        break;
      }

      if (millis() - Last_NTP_Update_Millis > 60 * 60 * Universal_1_sec_Interval) {
        Last_NTP_Update_Millis = millis(); //Actulizar la ultima hora de sincronizacion NTP
        fsm_state = STATE_UPDATE_TIME;
        break;
      }

      if ( millis() - RetardoLectura > 5 * Universal_1_sec_Interval) {
        OldTagRead = "1";
        RetardoLectura = millis(); //Actulizar la ultima hora de envio
      }

      // VERIFICAMOS CUANTAS VECES NO SE HAN ENVIOADO PAQUETES (ERRORES)
      if (failed >= fail_threshold) {
        failed = 0;
        published = 0;
        sent = 0;
        ESP.restart();
      }

      //verificar que el cliente de Conexion al servicio se encuentre conectado
      if (!client.connected()) {
        MQTTreconnect();
      }

      client.loop();
      yield();
          
    break;
    
    case STATE_TRANSMIT_BOTON_DATA:
      Serial.println(F("BOTON DATA SENT"));
      if (!client.connected()) {
        MQTTreconnect();
      }
      CheckTime();
      publish_Boton_Data();
      fsm_state = STATE_IDLE;
    break;

    case STATE_TRANSMIT_CARD_DATA:
      Serial.println(F("CARD DATA SENT"));
      if (!client.connected()) {
        MQTTreconnect();
      }
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
      //verificar que el cliente de Conexion al servicio se encuentre conectado
      if (!client.connected()) {
        MQTTreconnect();
      }
      //verificar la hora
      CheckTime();
      publishRF_ID_Manejo();
      fsm_state = STATE_IDLE;
    break;

    case STATE_UPDATE_TIME:
      Serial.println(F("NTP_CLIENT"));
      pTimeClient->update();
      {
        int ntpRetry = 0;
        while (NTP_response == false && ntpRetry < 5) {
          setSyncProvider(NTP_ready);                                                        //iniciamos la mensajeria de UDP para consultar la hora en el servicio de NTP remoto
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
  alarm.update();     // actualizar buzzer non-blocking
  ESP.wdtFeed();      // alimentar watchdog para evitar reinicio por bloqueo
  if (fsm_state != STATE_RDY_TO_UPDATE_OTA) {
    yield();
  }
}
