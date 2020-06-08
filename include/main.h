    #ifndef MAIN_H
    #define MAIN_H

    #ifdef ESP8266
    #include "ESP8266WiFi.h"       // Built-in
    #include "ESP8266WebServer.h"  // Built-in
    #else
        #include <WiFi.h>              // Built-in
        #include <WebServer.h>
        #include <ESPmDNS.h>
        #include <WiFiUdp.h>
        #include <SPIFFS.h>
        #include <ArduinoOTA.h>
    #endif
    #include "FS.h"



    #define VERSION 100
    #define DATA "08.06.2020"

    ///////////////MACROS///////////////////////
    #define S(msg) Serial.print(msg)
    #define Sn(msg) Serial.println(msg)

    #define T(msg) TelnetStream.print(msg)
    #define Tn(msg) TelnetStream.println(msg)

    #define B(msg) TelnetStream.print(msg) ; Serial.print(msg)
    #define Bn(msg) TelnetStream.println(msg) ; Serial.println(msg)


    #define FILE_NAME_WITH_CHANGES  "do.txt"
    #define SLEEP
   

    extern String do_text;
    extern String do_text_old;
    extern volatile int SleepModeCounter;

    extern bool sleep_on;

    void open_notes_file(String Notes_file_name);

      // interrrupt for the 2 sec tick to check in flash if new notes file has been uploaded 
    //extern  hw_timer_t * timer;
    //extern portMUX_TYPE timerMux;


    #endif