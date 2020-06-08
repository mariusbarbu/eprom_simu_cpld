  #ifndef HTML_H
  #define HTML_H

  #include <Arduino.h>
  #include "CSS.h"
  #include "MY_spiffs.h"
  #ifdef ESP8266
    #include "ESP8266WiFi.h"       // Built-in
    #include "ESP8266WebServer.h"  // Built-in
  #else
    #include <WiFi.h>              // Built-in
    #include <WebServer.h>
      #include <ESPmDNS.h>
      #include <WiFiUdp.h>
      #include <ArduinoOTA.h>
    #include <SPIFFS.h>
  #endif
  #include "FS.h"
  #include "main.h"

  #define ServerVersion "1.0"
  #define ACCESSPOINT
  #define WIFI


  extern bool    SPIFFS_present;
  extern String  webpage;


  #ifdef ACCESSPOINT
    extern const char *ssid_AP;
    extern const char *password_AP;
    extern IPAddress local_ip;
    extern IPAddress gateway;
    extern IPAddress subnet;
  #endif

  #ifdef WIFI
    extern const char ssid[];
    extern const char password[];
    extern IPAddress local_ip_WIFI;
    extern IPAddress gateway_WIFI;
    extern IPAddress subnet_WIFI;
  #endif


  #ifdef ESP8266
    extern ESP8266WebServer server;
  #else
    extern WebServer Webserver;
  #endif


void HomePage();
void File_Download();
void DownloadFile(String filename);
void File_Upload();
extern File UploadFile; 
void handleFileUpload();
void File_Stream();
void File_Delete();
void SendHTML_Header();
void SendHTML_Content();
void SendHTML_Stop();
void SelectInput(String heading1, String command, String arg_calling_name);
void ReportFileNotPresent(String target);
void ReportCouldNotCreateFile(String target);
extern String file_size(int bytes);


#endif
