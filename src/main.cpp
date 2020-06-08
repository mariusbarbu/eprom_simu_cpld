#include <Arduino.h>

/*  Version 4
 *  
 *  ESP32/ESP8266 example of downloading and uploading a file from or to the device's Filing System, including Directory and Streaming of files.
 *  
 This software, the ideas and concepts is Copyright (c) David Bird 2019. All rights to this software are reserved.
 
 Any redistribution or reproduction of any part or all of the contents in any form is prohibited other than the following:
 1. You may print or download to a local hard disk extracts for your personal and non-commercial use only.
 2. You may copy the content to individual third parties for their personal use, but only if you acknowledge the author David Bird as the source of the material.
 3. You may not, except with my express written permission, distribute or commercially exploit the content.
 4. You may not transmit it or store it in any other website or other form of electronic retrieval system for commercial purposes.

 The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the
 software use is visible to an end-user.
 
 THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT. FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY 
 OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 See more at http://www.dsbird.org.uk
 *
*/
// #define ESP8266

  #ifdef ESP8266
    #include "ESP8266WiFi.h"       // Built-in
    #include "ESP8266WebServer.h"  // Built-in
  #else
    #include <WiFi.h>              // Built-in
    #include <esp_wifi.h>
    #include <WebServer.h>
    #include <ESPmDNS.h>
    #include <WiFiUdp.h>
    #include <SPIFFS.h>
    #include <ArduinoOTA.h>
  #endif

  #include "FS.h"
  #include <CSS.h>
  #include <SPI.h>
  #include <html.h>
  #include <MY_spiffs.h>
  #include <main.h>
  #include <music.h>
  #include <TelnetStream.h>



 
  int ledPin = 14; 
  int led_state = 0;
  unsigned long my_time = 0;

  // interrrupt for the 2 sec tick to check in flash if new notes file has been uploaded 
  hw_timer_t * timer = NULL;
  portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

  #define INT_TIMER_DIVIDER 80          // for clk 80MHz we get one int each 80Mhz / 8000 = 10kHz = each 0.1ms
  #define INT_TIMER_MS 1000                 // every INT_TIMER_MS we get one int in the main. Every 0.1ms x 10 = 1msec
  #define SLEEP_DURATION 60000               // no of miliseconds till go to sleep, after a melody has been played.
                                          // As in sleep ftp and webserver are not working, we may upload files 
                                          // (fw and/or notes) only during melody play and/or this time out after a melody
  //int address0 = 32;                    // change on this pin will wake up

  /*
  #ifdef ACCESSPOINT
    WiFiServer APserver(80);
  #endif
  */

  volatile int interruptCounter;
  int totalInterruptCounter;
  volatile int SleepModeCounter;
  volatile bool InterruptFlag;

  bool sleep_on;




  void IRAM_ATTR onTimer() {
    portENTER_CRITICAL_ISR(&timerMux);
    //interruptCounter++;
    InterruptFlag = true;
    #ifdef SLEEP    // in main.h
     SleepModeCounter++;
    #endif
    portEXIT_CRITICAL_ISR(&timerMux);
  
  }

  void setup(void){
    Serial.begin(115200);
    delay(1000);

    #ifdef ACCESSPOINT  // in html.h
      // Connect to Wi-Fi network with SSID and password
      S("Setting AP (Access Point)…");
      // Remove the password parameter, if you want the AP (Access Point) to be open
      WiFi.softAP(ssid_AP, password_AP);
      delay(100);
      WiFi.softAPConfig(local_ip, gateway, subnet);
      delay(1000);
      WiFi.persistent(false);
      IPAddress IP = WiFi.softAPIP();
      S("AP IP address: ");
      Sn(IP);
      //APserver.begin();
    #endif
    
    #ifdef WIFI         // in html.h
    // Configures static IP address
      if (!WiFi.config(local_ip_WIFI, gateway_WIFI, subnet_WIFI) ) {
         Serial.println("STA Failed to configure");
      }
      WiFi.begin(ssid,password);
      while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
        delay(250); Serial.print('.');
      }
      delay(1000);
      Serial.println("\nConnected to "+WiFi.SSID()+" Use IP address: "+WiFi.localIP().toString()); // Report which SSID and IP is in use
      delay(1000);
    #endif
  
    #ifdef ESP8266
    if (!SPIFFS.begin()) {
    #else
    if (!SPIFFS.begin(true)) {
    #endif  
      Serial.println("SPIFFS initialisation failed...");
      SPIFFS_present = false; 
      delay(100);
    }
    else
    {
      Serial.println(F("SPIFFS initialized... file access enabled..."));
      SPIFFS_present = true; 
      delay(100);
    }

    // ******** OTA *********** 
    // Port defaults to 3232
    // ArduinoOTA.setPort(3232);

    // Hostname defaults to esp3232-[MAC]
    ArduinoOTA.setHostname("myesp32");
    // No authentication by default
    // ArduinoOTA.setPassword("admin");

    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
    ArduinoOTA
      .onStart([]() {
        SleepModeCounter = 0;
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        SleepModeCounter = 0;
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
      });

    ArduinoOTA.begin();

    TelnetStream.begin();


    //----------------------------------------------------------------------   
    ///////////////////////////// Server Commands 
    Webserver.on("/",         HomePage);
    Webserver.on("/download", File_Download);
    Webserver.on("/upload",   File_Upload);
    Webserver.on("/fupload",  HTTP_POST,[](){ Webserver.send(200);}, handleFileUpload);
    Webserver.on("/stream",   File_Stream);
    Webserver.on("/delete",   File_Delete);
    Webserver.on("/dir",      SPIFFS_dir);
    
    ///////////////////////////// End of Request commands
    Webserver.begin();
    Serial.println("HTTP server started");
    delay(100);

    // set the GPIOs registers for the addresses and data
    ini_gpio(); 
    // load the default melody inside the buffer
    initialize_melody();
    S("melody initialized with file: ");
    Sn(INI_NOTES_FILE);
    Sn("You may change the notes by uploading a new file , with different name, ");
    S("and put the name of the new file as text in the text file: ");
    Sn(FILE_NAME_WITH_CHANGES);
    Sn("and then uploaded this file to ESP32 too, via the webserver");
    delay(100);

    // initiliaze and start interrupt
    timer = timerBegin(0, INT_TIMER_DIVIDER, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, INT_TIMER_MS, true);
    timerAlarmEnable(timer);
    interruptCounter=0;
    SleepModeCounter=0;
    InterruptFlag = false;
    sleep_on = false;
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  void loop(void){
    /*
    #ifdef ACCESSPOINT
      WiFiClient client = APserver.available();   // Listen for incoming clients
        if (client) {                             // If a new client connects,
        Serial.println("New Client.");          // print a message out in the serial port
      }
      //if  (client.stop()){
      // Serial.println("Client disconnected.");
      // Serial.println("");
      //}
    #endif
    */
    Webserver.handleClient(); // Listen for client connections
    ArduinoOTA.handle();  
    HandleMusic();
    //delay(10);

    if (InterruptFlag) {
      portENTER_CRITICAL(&timerMux);
      //interruptCounter--;
      InterruptFlag = false;
      portEXIT_CRITICAL(&timerMux);
    }
    
    if (SleepModeCounter > SLEEP_DURATION) {
      portENTER_CRITICAL(&timerMux);
      SleepModeCounter=0;
      portEXIT_CRITICAL(&timerMux);
      if (digitalRead(22) == LOW){
        gpio_wakeup_enable(GPIO_NUM_22, GPIO_INTR_HIGH_LEVEL);
      }
      else{
        gpio_wakeup_enable(GPIO_NUM_22, GPIO_INTR_LOW_LEVEL);
      }
      esp_sleep_enable_gpio_wakeup();
      Serial.println("going to light sleep. Good night!");
      delay(10);
      //int ret = esp_light_sleep_start();
      esp_wifi_stop();
      sleep_on = true;
      esp_light_sleep_start();
      //Serial.printf("light_sleep: %d\n", ret); 
    }
  }


