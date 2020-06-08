#ifndef MY_SPIFFS_H
#define MY_SPIFFS_H

#include <Arduino.h>
#include <CSS.h>
#include <html.h>
#ifdef ESP8266
  #include "ESP8266WiFi.h"       // Built-in
  #include "ESP8266WebServer.h"  // Built-in
#else
  #include <WiFi.h>              // Built-in
  #include <WebServer.h>
  #include <SPIFFS.h>
  #include <ESPmDNS.h>
  #include <WiFiUdp.h>
  #include <ArduinoOTA.h>
#endif
#include <FS.h>
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#ifdef ESP32
void SPIFFS_dir();

void printDirectory(const char * dirname, uint8_t levels);

#endif
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#ifdef ESP8266
void SPIFFS_dir();
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SPIFFS_file_stream(String filename);

void SPIFFS_file_delete(String filename);

void ReportSPIFFSNotPresent();

#endif