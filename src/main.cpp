//File Sistema for Json Configuration File
#include <FS.h>                                                                                     //Libreria de incio del Sistema de Archivos (File Sistem) para el almacenamiento de variables de entorno
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

//**********************************************************************************FIN DE DEFINICION DE LIBRERIAS EXTERNAS
//Estructura de infromacion de ambiente de configuracion
struct BtnConf {
    char  MQTT_Server [30];                                                                          //Variable de Direccion de servidor de MQTT
    char  MQTT_Port [6] = "1883";                                                                    //Variable de puerto servidor de MQTT
    char  MQTT_User[24];                                                                             //Variable de usuario para conexion con servidor de MQTT
    char  MQTT_Password[24];                                                                         //Variable de Contraseña para conexion con servidor de MQTT
    char NTPClient_SERVER[24] = "time-a-g.nist.gov";                                                //Variable de Direccion de Servidor de NTP
    char  NTPClient_interval [6];                                                                    //Variable de intervalo de sincronizacion de hora por medio de NTP
};

BtnConf btnconfig;                                                                                  //Objeto de estructura de configuracion
//--------------------------------------------------------------------------------------------------//OTA && WifiManager
bool OTA_ENABLED = false;
bool Wifi_On_Demand_ENABLED = false;
unsigned long Btn_check_Current_millis;
//--------------------------------------------------------------------------------------------------incializacion del software Serial
SoftwareSerial RFIDReader(D2, D3, false, 256);                                                      //se inciializa el puerto Serial por Software en los puerstos D2 como TX y D3 como RX 
unsigned long RetardoLectura;                                                                       //Variable que almacena el tiempo transcurrido desde la ultima vez que se leyo una tarjeta 
long LecturaTreshold = 5000;                                                                        //Variable que almacena el tiempo que debe transcurrir en millisegundos para leer otra tarjeta
//--------------------------------------------------------------------------------------------------Variables Propias del CORE ESP8266 Para la administracion del Modulo
String NodeID = String(ESP.getChipId());                                                            //Variable Global que contiene la identidad del nodo (ChipID) o numero unico
//--------------------------------------------------------------------------------------------------definicion de parametros del Wifi
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;                                           //Variable de Identificacion de Cliente para servicio de MQTT Bluemix
String  Smacaddrs = "00:00:00:00:00:00";                                                            //Variable apara almacenar la MAC Address asignada por el servicio de Wifi
String  Sipaddrs  = "000.000.000.000";                                                              //Variable para almacenar la direccion IP asignada por el servicio de wifi
//--------------------------------------------------------------------------------------------------definicon de variables para NTP
WiFiUDP ntpUDP;                                                                                     //Cliente de Servicio UDP
boolean NTP_response = false;                                                                       //Bandera de comprobacion de lectura exitosa del servicio de Hora por internet 
String ISO8601;                                                                                     //Variable para almacenar la marca del timepo (timestamp) de acuerdo al formtao ISO8601
NTPClient timeClient("129.6.15.28", 0, atoi(btnconfig.NTPClient_interval));            //Cliente de conexion al servicio de hora por internet
//TESTING NTPClient timeClient(btnconfig.NTPClient_SERVER, 0, atoi(btnconfig.NTPClient_interval));            //Cliente de conexion al servicio de hora por internet

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
//--------------------------------------------------------------------------------------------------//variables Globales de lectura de codigos RFID
#define DataLenght 10
#define TypesofCards 13

byte incomingdata;
String inputString;
char SearchSValue;
byte tagID[DataLenght];
char charBuff[DataLenght];
boolean readedTag = false;
unsigned int count = 0;
String msg = "";
//--------------------------------------------------------------------------------------------------//variables Globales de lectura de eventos de boton
String IDE_ventoB;
int IdEventoB = 0;
//--------------------------------------------------------------------------------------------------//variables Globales para reinicio de hardware cada 24 horas (pendiente de cambio por time EPOCH)
unsigned long lastNResetMillis;                                                                      //Variable para llevar conteo del tiempo desde la ultima publicacion
//--------------------------------------------------------------------------------------------------//variables Globales de Core de Wifi para envio de estado de boton

int WifiSignal;
String Core_Version = ESP.getCoreVersion();
char SDK_version = ESP.getSdkVersion();
unsigned int CPU_Freq = ESP.getCpuFreqMHz();
unsigned int Sketch_Size = ESP.getSketchSize();

//**********************************************************************************FINITE_STATE_MACHINE_STATES:
int fsm_state;

//--------------------------------------------------------------------------------------------------Finite State Machine States
#define STATE_IDLE                    0
#define STATE_TRANSMIT_BOTON_DATA     1
#define STATE_TRANSMIT_CARD_DATA      2
#define STATE_RDY_TO_UPDATE_OTA       7

//**********************************************************************************FIN DE DEFINICION DE VARIABLES GLOBALES
//--------------------------------------------------------------------------------------------------callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Deberia Guardar la configuracion");
  shouldSaveConfig = true;
}

//--------------------------------------------------------------------------------------------------definicion de funcion que verifica y syncroniza la hora local con la hora del servicio de internet
time_t NTP_ready(){
  Serial.println(F("Transmit NTP Request"));                                                        //Mensaje Serial para verificacion del llamado de funcion en caso de falla
  timeClient.update();                                                                              //Funcion de la Libreria Externa que se llama para actulizar la lectura con el servicios de hora de internet
  uint32_t beginWait = millis();                                                                    //Variable para almacenar el momento en el que se empieza la espera de respuesta del servidor de hora de internet
  while (millis() - beginWait < 1500 ) {                                                            //Funcion en la que se define un periodo maximo de espera para la sincornizacion de hora de intarnet

    Serial.println(".");                                                                            //Mensaje Serial para verificacion de la lectura de tiempo UNIX del servicio de hora de internet
    delay(Universal_1_sec_Interval);
    unsigned long EpochTime = timeClient.getRawTime();                                            //Alamcenamiento de la hora UNIX de forma local

    if( EpochTime >= 3610){                                                                         //condicional comparativo para confirmar Si la hora contiene un formato mas grande  cualquier cosa random
      Serial.println(F("Receive NTP Response"));                                                    //Mensaje Serial para la verificacion de ejecucion del la condicional compartiva
      NTP_response = true;                                                                          //Actualizacion de la bandera de ejecucion de syncronizacion con hora de internet
      return EpochTime + (timeZone * SECS_PER_HOUR);                                                //Calculo de la hora tomando en cuenta la zona horaria 
    }
  }
  Serial.println(F("No NTP Response :-("));                                                         //Mensaje Serial de control en caso de falla, que el tiempo transcurrido para respuesta desde el servicio de hora de internet sea mayor al de tolerancia de respuesta
  NTP_response = false;                                                                             //Actulizacion de bandera de lectura exita de Servicio de hora de internet a falso
  return 0;                                                                                         //Salida del programa sin retorno de la hora
}
//--------------------------------------------------------------------------------------------------definicion de Funcion que incia y escribe el JSON con los parametros de Configuracion de ambiente de operacion
void Read_Configuration_JSON(){
  Serial.println(F("Montando el Sistema de Archivos o FS"));                                        //Mensaje Serial de control para verificar que la rutina ha iniciado
  if (SPIFFS.begin()){                                                                              //Condicional que verifica si se puede iniciar la libreria de Sistema de Archivos ó FS
    Serial.println(F("Sistema de Archivos montado"));                                               //Mensaje Serial de control para idicar que el sistema ha sido iniciado con exito
    if(SPIFFS.exists("/config.json")){                                                              //Condicional que verifica si en el sistema de Arachivos se encuentra creado el archivo de configuracion
      Serial.println(F("Leyendo el Archivo de Configuracion"));                                     //Mensaje Serial de control para idicar que el archivo de configuracionh ha sido encontrado con exito
      File configFile = SPIFFS.open("/config.json","r");                                            //Se Abre el archivo de configuracion
      if(configFile){                                                                               //condiconal que verifica si el archivo de configuracion pudo ser abierto
        Serial.println(F("Archivo de configuracion Abierto"));                                      //Mensaje Serial de control para idicar que el archivo de configuracionh ha sido abierto con exito
        size_t size = configFile.size();                                                            //averiguar cuanto mide el archivo y asinar ese paramtro a una variable
        std::unique_ptr<char[]>buf(new char [size]);                                                //Crear un Buffer para almacenar el contenido del archivo
        configFile.readBytes(buf.get(), size);                                                      //Leer el archivo y copiar el buiffer

        StaticJsonBuffer<512> jsonBuffer;                                                           //Creamos un Buffer estatico Grande para almacenar la informacion del JSON
        JsonObject &root = jsonBuffer.parseObject(buf.get());                                       //Almacenamos lo que haya en el el buffer del Archivo en el buffer del JSON
        if (!root.success()){                                                                       //Condicional de lectura del buffer
          Serial.println(F("Fallo en leer el archivo"));                                            //Mensaje Serial de control para idicar que el archivo de configuracionh NO ha sido abierto con exito
          return;
        }
        root.printTo(Serial);                                                                       //Imprimir el archivo de configuracion leido al puerto Serial para verificacion
        Serial.println(F("\nparsed json"));                                                         //Mensje serial para indicar que se empieza el parseo del archivo al Json
        strlcpy(btnconfig.MQTT_Server,                                                              // Copiar a <- destino
                root["MQTT_Server"] | "adnode.flatbox.io" ,                                                                // desde el valor <- Archivo
                sizeof( btnconfig.MQTT_Server) );                                                   // <- Capacidad del destino
        strlcpy(btnconfig.MQTT_Port, root["MQTT_Port"] | "1883", sizeof(btnconfig.MQTT_Port));      //Parametro de Configuracion del puerto del servidor de MQTT
        strlcpy(btnconfig.MQTT_User,root["MQTT_User"] | "demo", sizeof(btnconfig.MQTT_User));       //Parametro de Configuracion del Usuario del Servidor de MQTT
        strlcpy(btnconfig.MQTT_Password,root["MQTT_Password"] | "demo" , sizeof(btnconfig.MQTT_Password));    //Parametro de Configuracion del Password del Servidor de MQTT
        strlcpy(btnconfig.NTPClient_SERVER,root["NTPClient_SERVER"] | "time-a-g.nist.gov", 
                sizeof(btnconfig.NTPClient_SERVER));                                                //Parametro de Configuracion del Servidor de NTP
        strlcpy(btnconfig.NTPClient_interval, root["NTPClient_interval"] | "60000", sizeof(btnconfig.NTPClient_interval));                                  //Parametro de Configuracion del Intervalo de actulizacion de hora por medio de consulta al Servicio de NTP
      }
      configFile.close();
    }
  }else {
    Serial.println(F("Fallo en Abrir el Sistema de Archivos"));                                     //Mensje serial para indicar que Fallo el montar el sistema de manejo de archivos
  }
}
//--------------------------------------------------------------------------------------------------//Boot to on demand WifiManager
void BOOT_TO_On_Demand_Wifi_Manager(){

  WiFiManagerParameter custom_mqtt_server("server","MQTT_Server",btnconfig.MQTT_Server,64);
  WiFiManagerParameter custom_mqtt_port("port","MQTT_Port",btnconfig.MQTT_Port,6);
  WiFiManagerParameter custom_mqtt_user("user","MQTT_User",btnconfig.MQTT_User,64);
  WiFiManagerParameter custom_mqtt_pass("password","MQTT_Password",btnconfig.MQTT_Password,64);
  WiFiManagerParameter custom_NTPClient_SERVER("NTPServer","NTPClient_SERVER",btnconfig.NTPClient_SERVER,64);
  WiFiManagerParameter custom_NTPClient_interval("NTPInterval","NTPClient_interval",btnconfig.NTPClient_interval,6);
  
  WiFiManager wifiManager;

  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_NTPClient_interval);
  wifiManager.addParameter(&custom_NTPClient_SERVER);
  
  Serial.println(F("Empezando Configuracion de WIFI Bajo Demanda"));
  Purpura.COn();
  alarm.Beep(tono_medio);
  alarm.Beep(tono_medio);
  if (!wifiManager.startConfigPortal("flatwifi")) {
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5 * Universal_1_sec_Interval);
  }

  strcpy(btnconfig.MQTT_Server, custom_mqtt_server.getValue());
  strcpy(btnconfig.MQTT_Port, custom_mqtt_port.getValue());
  strcpy(btnconfig.MQTT_User, custom_mqtt_user.getValue());
  strcpy(btnconfig.MQTT_Password, custom_mqtt_pass.getValue());
  strcpy(btnconfig.NTPClient_SERVER,custom_NTPClient_SERVER.getValue());
  strcpy(btnconfig.NTPClient_interval,custom_NTPClient_interval.getValue());

  if(shouldSaveConfig){
    Serial.println(F("Guardando Cambios de Configuracion"));
    StaticJsonBuffer<512> jsonBuffer;  
    JsonObject& root = jsonBuffer.createObject();
    root["MQTT_Server"] = btnconfig.MQTT_Server;
    root["MQTT_Port"] = btnconfig.MQTT_Port;
    root["MQTT_User"] = btnconfig.MQTT_User;
    root["MQTT_Password"] = btnconfig.MQTT_Password;
    root["NTPClient_SERVER"] = btnconfig.NTPClient_SERVER;
    root["NTPClient_interval"] = btnconfig.NTPClient_interval;

    File configFile = SPIFFS.open("/config.json", "w");
    if(!configFile){
      Serial.println(F("Fallo al intentar abrir el archivo de configuracion para escritura"));
      return;
    }

    root.printTo(Serial);
    root.printTo(configFile);
    configFile.close();
    //end Save
  }
}
//--------------------------------------------------------------------------------------------------//Boot to on demand WifiManager
void BOOT_TO_Wifi_Manager(){
  
  WiFiManagerParameter custom_mqtt_server("server","MQTT_Server",btnconfig.MQTT_Server,64);
  WiFiManagerParameter custom_mqtt_port("port","MQTT_Port",btnconfig.MQTT_Port,6);
  WiFiManagerParameter custom_mqtt_user("user","MQTT_User",btnconfig.MQTT_User,64);
  WiFiManagerParameter custom_mqtt_pass("password","MQTT_Password",btnconfig.MQTT_Password,64);
  WiFiManagerParameter custom_NTPClient_SERVER("NTPServer","NTPClient_SERVER",btnconfig.NTPClient_SERVER,64);
  WiFiManagerParameter custom_NTPClient_interval("NTPInterval","NTPClient_interval",btnconfig.NTPClient_interval,6);
  
  WiFiManager wifiManager;

  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_NTPClient_interval);
  wifiManager.addParameter(&custom_NTPClient_SERVER);
  
  Serial.println(F("Empezando Configuracion de WIFI en Automatico"));
  Purpura.COn();
  if (!wifiManager.autoConnect("flatwifi")) {
    alarm.Beep(tono_corto);
    alarm.Beep(tono_corto);
    Purpura.CFlash(flash_medio);
    if (!wifiManager.startConfigPortal("flatwifi")) {
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5 * Universal_1_sec_Interval);
    }
  }

  strcpy(btnconfig.MQTT_Server, custom_mqtt_server.getValue());
  strcpy(btnconfig.MQTT_Port, custom_mqtt_port.getValue());
  strcpy(btnconfig.MQTT_User, custom_mqtt_user.getValue());
  strcpy(btnconfig.MQTT_Password, custom_mqtt_pass.getValue());
  strcpy(btnconfig.NTPClient_SERVER,custom_NTPClient_SERVER.getValue());
  strcpy(btnconfig.NTPClient_interval,custom_NTPClient_interval.getValue());

  if(shouldSaveConfig){
    Serial.println(F("Guardando Cambios de Configuracion"));
    StaticJsonBuffer<512> jsonBuffer;  
    JsonObject& root = jsonBuffer.createObject();
    root["MQTT_Server"] = btnconfig.MQTT_Server;
    root["MQTT_Port"] = btnconfig.MQTT_Port;
    root["MQTT_User"] = btnconfig.MQTT_User;
    root["MQTT_Password"] = btnconfig.MQTT_Password;
    root["NTPClient_SERVER"] = btnconfig.NTPClient_SERVER;
    root["NTPClient_interval"] = btnconfig.NTPClient_interval;

    File configFile = SPIFFS.open("/config.json", "w");
    if(!configFile){
      Serial.println(F("Fallo al intentar abrir el archivo de configuracion para escritura"));
      return;
    }

    root.printTo(Serial);
    root.printTo(configFile);
    configFile.close();
    //end Save
  }
}
//--------------------------------------------------------------------------------------------------//funcion donde se define el inicio a Actulizacion por OTA
void BOOT_TO_OTA() {
  Serial.println("Starting OTA");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid_OTA, password_OTA);
  ArduinoOTA.begin();
  Serial.println("Ready");
  Azul.Flash(flash_medio);
  fsm_state = STATE_RDY_TO_UPDATE_OTA;
  alarm.Beep(tono_medio);
  alarm.Beep(tono_corto);
  alarm.Beep(tono_medio);
}

//--------------------------------------------------------------------------------------------------//Funcion Remota para manejar Actulizacion de parametros por la via remota (MQTT SERVER)
void handleUpdate(byte* payload) {                                                                  //La Funcion recibe lo que obtenga Payload de la Funcion Callback que vigila el Topico de subcripcion (Subscribe TOPIC)
  StaticJsonBuffer<300> jsonBuffer;                                                                 //Se establece un Buffer de 1o suficientemente gande para almacenar los menasajes JSON
  JsonObject& root = jsonBuffer.parseObject((char*)payload);                                        //Se busca la raiz del mensaje Json convirtiendo los Bytes del Payload a Caracteres en el buffer
  if (!root.success()) {                                                                            //Si no se encuentra el objeto Raiz del Json
    Serial.println(F("ERROR en la Letura del JSON Entrante"));                                      //Se imprime un mensaje de Error en la lectura del JSON
    return;                                                                                         //Nos salimos de la funcion
  }                                                                                                 //se cierra el condicional
  Serial.println(F("Update payload:"));                                                             //si se pudo encontrar la raiz del objeto JSON se imprime u mensje
  root.prettyPrintTo(Serial);                                                                       //y se imprime el mensaje recibido al Serial
  Serial.println();                                                                                 //dejamos una linea de pormedio para continuar con los mensajes de debugging
}

//--------------------------------------------------------------------------------------------------//Funcion remota para mandar a dormir el esp despues de enviar un RFID
void handleResponse (byte* payloadrsp) {
  StaticJsonBuffer<200> jsonBuffer;                                                                 //Se establece un Buffer de 1o suficientemente gande para almacenar los menasajes JSON
  JsonObject& root = jsonBuffer.parseObject((char*)payloadrsp);                                     //Se busca la raiz del mensaje Json convirtiendo los Bytes del Payload a Caracteres en el buffer
  if (!root.success()) {                                                                            //Si no se encuentra el objeto Raiz del Json
    Serial.println(F("ERROR en la Letura del JSON Entrante"));                                      //Se imprime un mensaje de Error en la lectura del JSON
    return;                                                                                         //Nos salimos de la funcion
  }                                                                                                 //se cierra el condicional

  Serial.println(F("Response payload:"));                                                           //si se pudo encontrar la raiz del objeto JSON se imprime u mensje
  root.printTo(Serial);                                                                             //y se imprime el mensaje recibido al Serial
  Serial.println();                                                                                 //dejamos una linea de pormedio para continuar con los mensajes de debugging
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
}
//--------------------------------------------------------------------------------------------------//definicion de Cliente WIFI para ESP8266 y cliente de publicacion y subcripcion
WiFiClient wifiClient;                                                                              //Se establece el Cliente Wifi
PubSubClient client(wifiClient);                                                                    //se establece el Cliente para el servicio MQTT
//--------------------------------------------------------------------------------------------------//funcion para conectar al servidor de MQTT
void mqttConnect() {
  //int port_parsed = String(btnconfig.MQTT_Port).toInt();
  client.setServer(btnconfig.MQTT_Server, atoi(btnconfig.MQTT_Port));
  client.setCallback(callback);
  if (!!!client.connected()) {                                                                      //Verificar si el cliente se encunetra conectado al servicio
    Serial.print(F("Reconcetando el servidor MQTT: "));                                             //Si no se encuentra conectado imprimir un mensake de error y de reconexion al servicio
    Serial.println(String(btnconfig.MQTT_Server));                                                  //Imprimir la direccion del servidor a donde se esta intentado conectar
    char charBuf[30];
    String CID (clientId + NodeID);
    CID.toCharArray(charBuf, 30);
    while (!!!client.connect(charBuf, btnconfig.MQTT_User,btnconfig.MQTT_Password)) {               //Si no se encuentra conectado al servicio intentar la conexion con las credenciales Clientid, Metodo de autenticacion y el Tokeno password
      Serial.print(F("."));                                                                         //imprimir una serie de puntos mientras se da la conexion al servicio
      Blanco.CFlash(flash_corto);
    }
    Serial.println();                                                                               //dejar un espacio en la terminal para diferenciar los mensajes.
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
  
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& d = root.createNestedObject("d");
  JsonObject& metadata = d.createNestedObject("metadata");
  metadata["Universal_Interval"] = Universal_1_sec_Interval;
  metadata["UPDATE_TIME"] = 60*Universal_1_sec_Interval;
  metadata["Norman_Reset_TIME"] = 60*60*Universal_1_sec_Interval;
  metadata["timeZone"] = timeZone;    
  JsonObject& supports = d.createNestedObject("supports");
  supports["deviceActions"] = true;  
  JsonObject& deviceInfo = d.createNestedObject("deviceInfo");
  deviceInfo["NTP_Server"] = btnconfig.NTPClient_SERVER;
  deviceInfo["MQTT_server"] = btnconfig.MQTT_Server;
  deviceInfo["MacAddress"] = Smacaddrs;
  deviceInfo["IPAddress"]= Sipaddrs;    
  char buff[500];
  root.printTo(buff, sizeof(buff));
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
  Blanco.COff();                                                                                    //enviamos este comando para mandar todos los pines de RGB a LOW
  //inciamos los Seriales de hardware
  Serial.begin(115200);                                                                             //inciamos el puerto Serial de hardware a la velosidad indicada (def:15200)
  Serial.println(F("inicio exitosamnte el puerto Serial"));                                         //Mensaje Serial para la verificacion del incio del puerto serial
  Serial.println(F(""));                                                                            //Linea de Mensaje Serial intencionalmente dejada en Blanco para facilidad de lectura de mensajes Seriales
  //inciamos los Seriales de software
  RFIDReader.begin(9600);                                                                           //inciamos el puerto Serial por Software a la velocidad indicada (def: 9600)
  //leemos los parametros de configuracion almacenados en la memoria en el JSON de configuracion
  Read_Configuration_JSON();
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
    while ( WiFi.status() != WL_CONNECTED ) {
      BOOT_TO_Wifi_Manager();
      delay(Universal_1_sec_Interval);
      Serial.print ( "." );
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
    timeClient.begin();

    if(WiFi.status() == WL_CONNECTED){
      while (NTP_response == false){
        setSyncProvider(NTP_ready);
        delay(5*Universal_1_sec_Interval);
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
    Serial.print(F("            Servidor de MQTT: "));
    Serial.println(btnconfig.MQTT_User);
    Serial.print(F("            Client ID: "));
    Serial.println(clientId);
    delay(Universal_1_sec_Interval);
    // put your setup code here, to run once:
    Blanco.COff();
    fsm_state = STATE_IDLE; //inciar el estado del la maquina de stado finito
    yield();
  }
}
//****************************************************************************Inicio de funciones ejecutadas en el loop
//----------------------------------------------------------------------------funcion que procesa como desplegar y transmitir la hora de acuerdo al formato del ISO8601
void CheckTime(){ //digital clock display of the time
  time_t prevDisplay = 0; 
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) {                                             //update the display only if time has changed
      prevDisplay = now();
      ISO8601 = String (year(), DEC);
      ISO8601 += "-";
      ISO8601 += month();
      ISO8601 += "-";
      ISO8601 += day();
      ISO8601 +="T";
      if ((hour() >= 0)&& (hour() < 10)){
        //Serial.print(F("+0:"));
        //Serial.println(hour());
        ISO8601 +="0";
        ISO8601 += hour();
      }else{
        //Serial.print(F("hora:"));
        //Serial.println(hour());
        ISO8601 += hour();
      }
      ISO8601 += ":";
      ISO8601 += minute();
      ISO8601 += ":";
      ISO8601 += second();
    }
  }
  Serial.println(F("Time not Sync, Syncronizing time"));
  NTP_ready();
}
//---------------------------------------------------------------------------------------------------Leer la tarjeta que se presenta
void readTag() {
  if (RFIDReader.available()) {
    if (RFIDReader.available() == 0x02) {                                                           //if data header is present.
      while (RFIDReader.available() > 0) {                                                          // If data available from reader
        incomingdata = RFIDReader.read();
        /*Serial.print(count);
           Serial.print(F(":HEX:"));
           Serial.print(incomingdata,HEX);
           Serial.print(F(":DEC:"));
           Serial.print(incomingdata,DEC);
           Serial.println(F(""));*/
        tagID[count] = incomingdata;
        if (count > 3 && count < 8) {
          inputString += incomingdata;
        }
        delay(10);
        if (count == 12) break;
        count++ ;
      }
      Serial.print(F("RFID CARD ID IS: "));
      Serial.println(inputString);
      Verde.Flash(flash_corto);
      alarm.Beep(tono_corto);
      fsm_state = STATE_TRANSMIT_CARD_DATA;
      //inputString = "";
      count = 0;
      readedTag = !readedTag;
    }
  }
  return;
}
//---------------------------------------------------------------------------------------------- fucnion de lectura de activiad del boton
void readBtn() {
  if (T_button.check == true){
    Serial.println("Pressed");
    IdEventoB ++;
    IDE_ventoB = String (NodeID + IdEventoB);
    Azul.Flash(flash_corto);
    alarm.Beep(tono_corto);
    fsm_state = STATE_TRANSMIT_BOTON_DATA; //PUTS FSM MACHINE ON TRANSMIT DATA MODE
  }
}

//-------- Data de Manejo RF_ID_Manejo. Publish the data to MQTT server, the payload should not be bigger than 45 characters name field and data field counts. --------//
void publishRF_ID_Manejo (String IDModulo, String MSG, float vValue, int RSSIV, int env, int fail, String Tstamp, String SMacAd, String SIpAd) {
  StaticJsonBuffer<300> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& d = root.createNestedObject("d");
  JsonObject& Ddata = d.createNestedObject("Ddata");
  Ddata["ChipID"] = IDModulo;
  Ddata["Msg"] = MSG;
  Ddata["batt"] = vValue;
  Ddata["RSSI"] = RSSIV;
  Ddata["publicados"] = env;
  Ddata["enviados"] = sent;
  Ddata["fallidos"] = fail;
  Ddata["Tstamp"] = Tstamp;
  Ddata["Mac"] = SMacAd;
  Ddata["Ip"] = SIpAd;
  char MqttDevicedata[300];
  root.printTo(MqttDevicedata, sizeof(MqttDevicedata));
  Serial.println(F("publishing device data to manageTopic:"));
  Serial.println(MqttDevicedata);
  sent++;
  if (client.publish(manageTopic, MqttDevicedata)) {
    Serial.println(F("enviado data de dispositivo:OK"));
    published ++;
    failed = 0;
  } else {
    Serial.print(F("enviado data de dispositivo:FAILED"));
    failed ++;
  }
}

//------------------------------------------------------------------------------------------------Funcion de reseteo normal
void NormalReset() {
  if (millis() - lastNResetMillis > 60 * 60 * Universal_1_sec_Interval) {
    hora++;
    WifiSignal = WiFi.RSSI();
    if (hora > 24) {
      msg = ("24h NReset");
      VBat = 4.2; //Bateria();
      publishRF_ID_Manejo(NodeID, msg, VBat, WifiSignal, published, failed, ISO8601, Smacaddrs, Sipaddrs);        //publishRF_ID_Manejo (String IDModulo,String MSG,float vValue, int fail,String Tstamp)
      void disconnect ();
      hora = 0;
      ESP.restart();
    }
    lastNResetMillis = millis(); //Actulizar la ultima hora de envio
  }
}
  
//**************************************************************************************************INICIO DE FUNCIONES DE LOOP
//--------------------------------------------------------------------------------------------------Funcion de bucle infinito (Loop) este codigo se ejecuta repetitivamente
void loop() {
  //iniciamos el Switch de estado
  switch(fsm_state){                                                                                //se lee en que stado debe iniciar el Switch de estados
    case STATE_IDLE :                                                                               //Si el estado que se lee es el por defecto IDLE cunado el boton no hace nada 90% del tiempo
      readTag();                                                                                    //leer su hay alguna tarjeta
      readBtn();                                                                                    //leer si se presiono el boton
      NormalReset();
      checkalarms();
      LocalWarning ();

      if (millis() - lastUPDATEMillis > 30 * 60 * Universal_1_sec_Interval) {
        lastUPDATEMillis = millis(); //Actulizar la ultima hora de envio
        fsm_state = STATE_UPDATE;
      }

      if (millis() - lastUPDATEMillis > 60 * 60 * Universal_1_sec_Interval) {
        lastUPDATEMillis = millis(); //Actulizar la ultima hora de envio
        fsm_state = STATE_UPDATE_TIME;
      }

      if ( millis() - RetardoLectura > 5 * Universal_1_sec_Interval) {
        OldTagRead = "1";
        RetardoLectura = millis(); //Actulizar la ultima hora de envio
      }

      // VERIFICAMOS CUANTAS VECES NO SE HAN ENVIOADO PAQUETES (ERRORES)
      if (failed >= FAILTRESHOLD) {
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
    case STATE_RDY_TO_UPDATE_OTA:
      ArduinoOTA.handle();
      break;
  }
  if (fsm_state != STATE_RDY_TO_UPDATE_OTA) {
    yield();
  }
}
