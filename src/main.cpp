  #include <Arduino.h>
  #include <SPI.h>

  #include "ESP8266WiFi.h"       // Built-in
  #include "ESP8266WebServer.h"  // Built-in

  #include "FS.h"
  #include <LittleFS.h>
  #include "CSS.h"
  #include "html.h"
  #include "MY_spiffs.h"
  #include "main.h"
 
  // define Pins
  #define LED 5
  #define LED_ON HIGH
  #define LED_OFF LOW
  #define LED_BUILDIN_ON LOW
  #define LED_BUILDIN_OFF HIGH        // led buildin at esp12f = gpio2

  bool led_state= LED_ON;

  #define nSEN 12
  #define nOE 2
  #define nWE 4

  //#define MCLK 14
  //#define MOSI 13

  #define CPLD_MUX 16
  #define ADDRESS_ESP_TO_SRAM HIGH
  #define ADDRESS_PCB_TO_SRAM LOW

  #define WR 15
  #define WRITE_TO_SRAM HIGH
  #define READ_FROM_SRAM LOW


  // interrrupt for the 2 sec tick to check in flash if new notes file has been uploaded 
  //hw_timer_t * timer = NULL;
  //portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

  #define INT_TIMER_DIVIDER 80          // for clk 80MHz we get one int each 80Mhz / 8000 = 10kHz = each 0.1ms
  #define INT_TIMER_MS 1000                 // every INT_TIMER_MS we get one int in the main. Every 0.1ms x 10 = 1msec
  #define SLEEP_DURATION 60000               // no of miliseconds till go to sleep, after a melody has been played.
                                          // As in sleep ftp and webserver are not working, we may upload files 
                                          // (fw and/or notes) only during melody play and/or this time out after a melody
  //int address0 = 32;                    // change on this pin will wake up

  volatile int SleepModeCounter;
  String do_text;
  String do_text_old;
  uint16_t file_length;
  String FileName;
  uint32_t MemoryAddress;


  #define SPI_SPEED 1000000

  #define MELODY_LENGTH 512

  #define TWO_POWER_18 262144

  #define MAX_MEMORY_LENGTH 262144


  char melody[MELODY_LENGTH];
  char mel_test[MELODY_LENGTH] = {0x38, 0x3F, 0x3F, 0x30, 0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 
                                  0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00,
                                  0x6E, 0x00, 0x00, 0x6E, 0x6E, 0x00, 0x00, 0x6E, 0x6E, 0x00, 0x00, 0x6E, 0x6E, 0x00, 0x00, 0x6E, 
                                  0x6E, 0x00, 0x00, 0x6E, 0x6E, 0x00, 0x00, 0x6E, 0x6E, 0x00, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x00,
                                  0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 
                                  0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 0x00, 0x00, 0x00, 0x00,
                                  0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 
                                  0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00,
                                  0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 
                                  0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00,
                                  0x6E, 0x00, 0x00, 0x6E, 0x6E, 0x00, 0x00, 0x6E, 0x6E, 0x00, 0x00, 0x6E, 0x6E, 0x00, 0x00, 0x6E, 
                                  0x6E, 0x00, 0x00, 0x6E, 0x6E, 0x00, 0x00, 0x6E, 0x6E, 0x00, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x00, 
                                  0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 
                                  0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 0x00, 0x00, 0x00, 0x00, 
                                  0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 
                                  0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00,
                                  0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 
                                  0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 0x00, 0x00, 0x00, 0x00,
                                  0x5D, 0x00, 0x00, 0x5D, 0x5D, 0x00, 0x00, 0x5D, 0x5D, 0x00, 0x00, 0x5D, 0x5D, 0x00, 0x00, 0x5D, 
                                  0x5D, 0x00, 0x00, 0x5D, 0x5D, 0x00, 0x00, 0x5D, 0x5D, 0x00, 0x00, 0x5D, 0x00, 0x00, 0x00, 0x00,
                                  0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 
                                  0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x00, 
                                  0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 
                                  0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 0x00, 0x00, 0x00, 0x00,
                                  0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 
                                  0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 0x62, 0x00, 0x00, 0x62, 0x00, 0x00, 0x00, 0x00,
                                  0x5D, 0x00, 0x00, 0x5D, 0x5D, 0x00, 0x00, 0x5D, 0x5D, 0x00, 0x00, 0x5D, 0x5D, 0x00, 0x00, 0x5D, 
                                  0x5D, 0x00, 0x00, 0x5D, 0x5D, 0x00, 0x00, 0x5D, 0x5D, 0x00, 0x00, 0x5D, 0x00, 0x00, 0x00, 0x00,
                                  0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 
                                  0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 
                                  0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 
                                  0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 0x52, 0x00, 0x00, 0x52, 0xFF, 0xFF, 0xFF, 0xFF};

  void initialize_melody(){
  uint16_t j;
    
    Sn(" Initialize melody");
    for (j=0; j< MELODY_LENGTH; j++){
      melody[j]=mel_test[j];
    }
  //	load_notes();		// if spiffs fails we have only first melody as in const data
  }                      

  uint32_t concatenate( char data_in, uint32_t address){
    uint32_t data;
    uint32_t out;
    data = data_in;
    data = data << 18;
    out = data | address;
    return out;
  }
  void test_sram(uint32_t address, uint32_t lenght){
    char dummy = 0x00;
    uint32_t cpld_data;
    uint32_t j;
    char transfer;
    digitalWrite(CPLD_MUX, ADDRESS_ESP_TO_SRAM);
    digitalWrite(WR, READ_FROM_SRAM);
    for(j=0; j<lenght; j++){
      cpld_data = concatenate(dummy, address);
      transfer  = cpld_data>>24 & 0xFF;
      SPI.transfer(transfer);
      transfer  = cpld_data>>16 & 0xFF;
      SPI.transfer(transfer);
      transfer  = cpld_data>>8 & 0xFF;
      SPI.transfer(transfer);
      transfer  = cpld_data & 0xFF;
      SPI.transfer(transfer);
      address++;
      digitalWrite(LED, led_state);
      if(led_state == LED_ON){
        led_state = LED_OFF;
      }
      else{
        led_state = LED_ON;
      }
      delayMicroseconds(1);
    }
  }

  void write_sram( char *melody, uint32_t address, uint32_t lenght){
    S(" Write to SRAM at address ");
    S(address);
    S(" a number of ");
    S(lenght);
    Sn(" bytes");
    uint32_t cpld_data;
    uint32_t j;
    char transfer;
  //      digitalWrite(LED, LOW);
    for(j=0; j<lenght; j++){
      cpld_data = concatenate(melody[j], address);
      transfer  = cpld_data>>24 & 0xFF;
      SPI.transfer(transfer);
      transfer  = cpld_data>>16 & 0xFF;
      SPI.transfer(transfer);
      transfer  = cpld_data>>8 & 0xFF;
      SPI.transfer(transfer);
      transfer  = cpld_data & 0xFF;
      SPI.transfer(transfer);
      //digitalWrite(nWE, LOW);
      GPOC = 1<<nWE;
      delayMicroseconds(1);
      GPOS = 1<<nWE;
      //digitalWrite(nWE, HIGH);
      address++;
    }
    //   digitalWrite(LED, HIGH);

  }

  String load_file_from_initial(){
    String file_to_load = "";

    FileName = "/initial.txt";

    File file = SPIFFS.open(FileName, "r");
    if (!file) {
      Serial.println("Failed to open 'initial.txt' file for reading");
      return file_to_load;
    }   
    while (file.available()) {
      file_to_load += (char)file.read();
    }
    file.close();
    S("File: "); S(file_to_load); Sn(" was read from 'initial.txt'");
    return file_to_load;


  }

  void load_file_to_memory( String FileName){

    if(!FileName.startsWith("/")) {
      FileName = "/"+FileName;
    }
    File file = SPIFFS.open(FileName, "r");
    if (!file) {
      Serial.println("Failed to open file for reading");
      return;
    }
    MemoryAddress = 0x00;
    uint32_t StartMemoryAddress = MemoryAddress;
    S("File choosed: "); Sn(FileName);
    S("File size: "); S(file.size()); Sn(" bytes");
    if(((file.size() +MemoryAddress) > MAX_MEMORY_LENGTH)){
      Sn("!!!!!!! File + Start Address > Memory Space !!!!!!");
      Sn("!!!!!!! File was not loaded to Memory !!!!!!!!!");
      return;
    }
    uint32_t bytes_counter = 0;
    S("Now write to memory ");
    uint64_t NowMillis = millis();
    led_state = false;
    digitalWrite(LED, led_state);
    while (file.available()) {
      memory_write_with_increment(file.read());
      bytes_counter++;
      if((millis() -NowMillis)>1000){
        NowMillis = millis();
        S(".");
        led_state = !led_state;
        if(led_state) digitalWrite(LED, LED_ON);
        else          digitalWrite(LED, LED_OFF);
       }
    }
    Sn("");
    digitalWrite(LED, LED_ON);
    file.close();
    S("File: "); S(FileName);S(" loaded to memory beginning at address: 0x"); Serial.println(StartMemoryAddress, HEX);
    S("File length: "); S(bytes_counter); Sn(" bytes");
    S("Last memory address: 0x"); Serial.println(MemoryAddress, HEX);
  }

  void memory_write_one_byte_at_address (uint8_t ByteToWrite, uint32_t MemoryAddress){

    digitalWrite(CPLD_MUX, ADDRESS_ESP_TO_SRAM);
    digitalWrite(WR, WRITE_TO_SRAM);
    uint32_t cpld_data;
    char transfer;
    cpld_data = concatenate(ByteToWrite, MemoryAddress);
    transfer  = cpld_data>>24 & 0xFF;
    SPI.transfer(transfer);
    transfer  = cpld_data>>16 & 0xFF;
    SPI.transfer(transfer);
    transfer  = cpld_data>>8 & 0xFF;
    SPI.transfer(transfer);
    transfer  = cpld_data & 0xFF;
    SPI.transfer(transfer);
    digitalWrite(nWE, LOW);
    delayMicroseconds(1);
    digitalWrite(nWE, HIGH);

    digitalWrite(CPLD_MUX, ADDRESS_PCB_TO_SRAM);
    digitalWrite(WR, READ_FROM_SRAM);

  //  SPI.endTransaction();
  //test

      digitalWrite(nOE, LOW); // SRAM output enable 

  }

  void memory_write_with_increment(uint8_t ByteToWrite){
    
    memory_write_one_byte_at_address (ByteToWrite, MemoryAddress);
    MemoryAddress++;

  }
  void setup() {

    Serial.begin(115200);          // Start the Serial communication to send messages to the computer
    Serial.println("start");

    //digitalWrite(LED_BUILTIN, LED_BUILDIN_OFF);

    pinMode(LED, OUTPUT);
    digitalWrite(LED, LED_ON);

    pinMode(nSEN, OUTPUT);
    digitalWrite(nSEN, LOW);

    pinMode(nOE, OUTPUT);
    digitalWrite(nOE, LOW);

    pinMode(nWE, OUTPUT);
    digitalWrite(nWE, HIGH);

    pinMode(CPLD_MUX, OUTPUT);
    digitalWrite(CPLD_MUX, ADDRESS_ESP_TO_SRAM);

    pinMode(WR, OUTPUT);
    digitalWrite(WR, WRITE_TO_SRAM);

    //pinMode(MCLK, OUTPUT);
    //pinMode(MOSI, OUTPUT);

    initialize_melody();
    SPI.begin();
  // SPI.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE0));
  //SPI.setup(id, mode, cpol, cpha, databits, clock_div[, duplex_mode]);
    //digitalWrite(LED, LED_ON);
    write_sram(melody, 0, MELODY_LENGTH );
    //digitalWrite(LED, LED_OFF);

    digitalWrite(CPLD_MUX, ADDRESS_PCB_TO_SRAM);
    digitalWrite(WR, READ_FROM_SRAM);

  //  SPI.endTransaction();
  //test

      digitalWrite(nOE, LOW); // SRAM output enable 

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
      
      digitalWrite(LED, LED_ON);
      delay(250);
      digitalWrite(LED, LED_OFF);
      delay(250);
      digitalWrite(LED, LED_ON);
      delay(250);
      digitalWrite(LED, LED_OFF);
      delay(250);
      digitalWrite(LED, LED_ON);
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

    //----------------------------------------------------------------------   
    ///////////////////////////// Server Commands 
    Webserver.on("/",         HomePage);
    Webserver.on("/download", File_Download);
    Webserver.on("/upload",   File_Upload);
    Webserver.on("/fupload",  HTTP_POST,[](){ Webserver.send(200);}, handleFileUpload);
    Webserver.on("/stream",   File_Stream);
    Webserver.on("/delete",   File_Delete);
    Webserver.on("/dir",      SPIFFS_dir);
    Webserver.on("/userinput", userinput);  
  
    ///////////////////////////// End of Request commands
    Webserver.begin();
    Serial.println("HTTP server started");
    delay(100);
    digitalWrite(LED, LED_ON);

    load_file_to_memory(load_file_from_initial());
    //Sn("file written to memory");


  }

  void loop() {
    // put your main code here, to run repeatedly:
    //delay(1);
  //  test_sram(0, 512);
    Webserver.handleClient(); // Listen for client connections
    delay(1);
  }


