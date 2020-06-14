  #include <Arduino.h>
  #include "CSS.h"
  #include "html.h"
  #include "MY_spiffs.h"
  #ifdef ESP8266
    #include <ESP8266WiFi.h>       // Built-in
    #include <ESP8266WebServer.h>  // Built-in
  #else
 #include <WiFi.h>              // Built-in
  #include <WebServer.h>
    #include <ESPmDNS.h>
    #include <WiFiUdp.h>
    #include <ArduinoOTA.h>
  #include <SPIFFS.h>
  #endif
  #include "FS.h"
  #include <main.h>

  bool    SPIFFS_present = false;
  String  webpage = "";

  #ifdef ACCESSPOINT
    const char *ssid_AP     = "ESP32";
    const char *password_AP = "12345678";

    IPAddress local_ip(10,0,0,1);
    IPAddress gateway(10,0,0,1);
    IPAddress subnet(255,255,255,0);

  #endif
  #ifdef WIFI
    const char ssid[]     = "mb2000_2.4GHz";
    const char password[] = "resita99";
    IPAddress local_ip_WIFI(192,168,1,29);
    IPAddress gateway_WIFI(192,168,1,249);
    IPAddress subnet_WIFI(255,255,255,0);
  #endif

  #ifdef ESP8266
    ESP8266WebServer Webserver(80);
  #else
    WebServer Webserver(80);
   #endif
   WiFiClient myclient;

  File UploadFile; 
  String file_size(int bytes);


  String siteheading    = "EPROM SIMULATOR";               // Site's Main Title
  String subheading     = "Sensor Readings";               // Sub-heading for all pages
  String sitetitle      = "ESP32 Webserver";               // Appears on the tabe of a Web Browser
  String yourfootnote   = "Al.T.Oida"; // A foot note e.g. "My Web Site"
  String siteversion    = VERSION;  // Version of your Website
  String siteVersionDate = DATA;
  String MyCopyright    = "Oida";
  bool FileType = true;     // true = bin, false = hex

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// All supporting functions from here...
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void HomePage(){
  SleepModeCounter =0;
  Sn("Sleep postponed");
  SendHTML_Header();
  webpage += F("<a href='/download'><button>Download</button></a>");
  webpage += F("<a href='/upload'><button>Upload</button></a>");
  webpage += F("<a href='/stream'><button>Stream</button></a>");
  webpage += F("<a href='/delete'><button>Delete</button></a>");
  webpage += F("<a href='/dir'><button>Directory</button></a>");
  webpage += F("<a href='/userinput'><button>Select File</button></a>");

  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop(); // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Download(){ // This gets called twice, the first pass selects the input, the second pass then processes the command line arguments
  SleepModeCounter = 0;
  Sn("Sleep postponed");
  if (Webserver.args() > 0 ) { // Arguments were received
    if (Webserver.hasArg("download")) DownloadFile(Webserver.arg(0));
  }
  else SelectInput("Enter filename to download","download","download");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void DownloadFile(String filename){
  SleepModeCounter = 0;
  Sn("Sleep postponed");
  Serial.print("download file   ");
  if (SPIFFS_present) { 
    File download = SPIFFS.open("/"+filename,  "r");
  Serial.println(filename);
    if (download) {
      Webserver.sendHeader("Content-Type", "text/text");
      Webserver.sendHeader("Content-Disposition", "attachment; filename="+filename);
      Webserver.streamFile(download, "application/octet-stream");
      download.close();
      Webserver.sendHeader("Connection", "close");
        Serial.println("downloadc closed");

    } else ReportFileNotPresent("download"); 
  } else ReportSPIFFSNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Upload(){
  SleepModeCounter = 0;
  Sn("Sleep postponed");
  append_page_header();
  webpage += F("<h3>Select File to Upload</h3>"); 
  webpage += F("<FORM action='/fupload' method='post' enctype='multipart/form-data'>");
  webpage += F("<input class='buttons' style='width:40%' type='file' name='fupload' id = 'fupload' value=''><br>");
  webpage += F("<br><button class='buttons' style='width:10%' type='submit'>Upload File</button><br>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  Webserver.send(200, "text/html",webpage);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void compare_notes_files (String file_name){
  if (file_name == FILE_NAME_WITH_CHANGES){
    File do_file = SPIFFS.open("/"+file_name,  "r");
    do_text = "";
    while(do_file.available()){
        do_text += (char)do_file.read();
    }
    Sn(do_text);
    S(" old file ");
    Sn(do_text_old);
    if (do_text_old  == do_text){
      Sn(" new <do.txt> file but same notes file. Do nothing");
    }
    else{
        Sn("new <do.txt> file and new notes file");
        //open_notes_file(do_text);
        do_text_old =   do_text;
    }
  }
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//File UploadFile; 
void handleFileUpload(){ // upload a new file to the Filing system
  SleepModeCounter = 0;
  Sn("Sleep postponed");
  HTTPUpload& uploadfile = Webserver.upload(); // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/srcv
                                            // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
  if(uploadfile.status == UPLOAD_FILE_START)
  {
    String filename = uploadfile.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("Upload File Name: "); Serial.println(filename);
    SPIFFS.remove(filename);                  // Remove a previous version, otherwise data is appended the file again
    UploadFile = SPIFFS.open(filename, "w");  // Open the file for writing in SPIFFS (create it, if doesn't exist)
  }
  else if (uploadfile.status == UPLOAD_FILE_WRITE)
  {
    if(UploadFile) UploadFile.write(uploadfile.buf, uploadfile.currentSize); // Write the received bytes to the file
  } 
  else if (uploadfile.status == UPLOAD_FILE_END)
  {
    if(UploadFile)          // If the file was successfully created
    {                                    
      UploadFile.close();   // Close the file again
      Serial.print("Upload Size: "); Serial.println(uploadfile.totalSize);
      webpage = "";
      append_page_header();
      webpage += F("<h3>File was successfully uploaded</h3>"); 
      webpage += F("<h2>Uploaded File Name: "); webpage += uploadfile.filename+"</h2>";
      webpage += F("<h2>File Size: "); webpage += file_size(uploadfile.totalSize) + "</h2><br>"; 
      append_page_footer();
      Webserver.send(200,"text/html",webpage);
      //compare_notes_files(uploadfile.filename);
    } 
    else
    {
      ReportCouldNotCreateFile("upload");
    }
  }
}





//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Stream(){
  if (Webserver.args() > 0 ) { // Arguments were received
    if (Webserver.hasArg("stream")) SPIFFS_file_stream(Webserver.arg(0));
  }
  else SelectInput("Enter a File to Stream","stream","stream");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Delete(){
  SleepModeCounter = 0;
  Sn("Sleep postponed");
  if (Webserver.args() > 0 ) { // Arguments were received
    if (Webserver.hasArg("delete")) SPIFFS_file_delete(Webserver.arg(0));
  }
  else SelectInput("Select a File to Delete","delete","delete");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Header(){
  Webserver.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate"); 
  Webserver.sendHeader("Pragma", "no-cache"); 
  Webserver.sendHeader("Expires", "-1"); 
  Webserver.setContentLength(CONTENT_LENGTH_UNKNOWN); 
  Webserver.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves. 
  append_page_header();
  Webserver.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Content(){
  Webserver.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Stop(){
  Webserver.sendContent("");
  Webserver.client().stop(); // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SelectInput(String heading1, String command, String arg_calling_name){
  SendHTML_Header();
  webpage += F("<h3>"); webpage += heading1 + "</h3>"; 
  webpage += F("<FORM action='/"); webpage += command + "' method='post'>"; // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!
  webpage += F("<input type='text' name='"); webpage += arg_calling_name; webpage += F("' value=''><br>");
  webpage += F("<type='submit' name='"); webpage += arg_calling_name; webpage += F("' value=''><br><br>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportFileNotPresent(String target){
  SendHTML_Header();
  webpage += F("<h3>File does not exist</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportCouldNotCreateFile(String target){
  SendHTML_Header();
  webpage += F("<h3>Could Not Create Uploaded File (write-protected?)</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
String file_size(int bytes){
  String fsize = "";
  if (bytes < 1024)                 fsize = String(bytes)+" B";
  else if(bytes < (1024*1024))      fsize = String(bytes/1024.0,3)+" KB";
  else if(bytes < (1024*1024*1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
  else                              fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";
  return fsize;
}

void userinput() {
  FileType = false;
  //buildUserInputPage();
  myclient = Webserver.client();
  if (Webserver.args() > 0 ) { // Arguments were received
    for ( uint8_t i = 0; i < Webserver.args(); i++ ) {
      String Argument_Name   = Webserver.argName(i);
      String client_response = Webserver.arg(i);
      if (Argument_Name == "File Name") {
        FileName = client_response;
        S("File selected: "); Sn(FileName);
        load_file_to_memory(FileName);
      }
      if(Argument_Name == "FileType") {
        FileType = client_response;
        Serial.println("Checkbox choice was : "+FileType);
      }
    }
  }
  /*
  UR_response.trim(); // Remove any leading spaces
  if (isValidNumber(UR_response)) UR = UR_response.toFloat();
  Voltage_Value = UR;
  Serial.println("   UR : "+String(UR,1));
  */
  //Serial.println("Checkbox choice was : "+Precharge);
   buildUserInputPage();
  myclient.println(webpage);
 //showInput();
}

void buildUserInputPage(){
  //Precharge = "";
  webpage = ""; // don't delete this command, it ensures the server works reliably!
  append_page_header();
  #ifdef ACCESSPOINT
  String IPaddress = local_ip.toString();
  #else
  String IPaddress = WiFi.localIP().toString();
  #endif
  
  webpage += "<h3>Please input the Eprom file from device memory to be simulate</h3>";
  webpage += "<form action=\"http://"+IPaddress+"/userinput\" method=\"POST\">";
  webpage += "<table style='font-family:arial,sans-serif;font-size:16px;border-collapse:collapse;text-align:center;width:90%;margin-left:auto;margin-right:auto;'>";
  webpage += "<tr>";    // tablw row
  webpage += "<th style='border:0px solid black;text-align:left;padding:2px;'>File Name</th>";  // table column
  webpage += "</tr>";   // end table row
  
  webpage += "<tr>";
  webpage += "<td style='border:0px solid black;text-align:left;padding:2px;'><input type='text' name='File Name' value= " + FileName+"></td>";
  //webpage += "<td style='border:0px solid black;text-align:left;padding:2px;'><input type='text' name='Releu' value= " + RelaisType +"></td>";
  webpage += "</tr>";
  
  webpage += "<tr>";
  webpage += "</table><br><br>";
  webpage += "<table style='font-family:arial,sans-serif;font-size:16px;border-collapse:collapse;text-align:center;width:90%;margin-left:auto;margin-right:auto;'>";
  webpage += "<tr>";    // tablw row
  webpage += "</table><br><br>";

  webpage += "<input type='checkbox' name='File Type: BIN(Y) or HEX(N)' value='Y'>FileType"; 
  // And so-on  
  webpage += "<br><br><input type='submit' value='Enter'><br><br>";
  webpage += "</form></body>";
  /*
  webpage += "<p>This page was displayed on : "+GetTime(timeinfo)+" Hr</p>";
  Sn(GetTime(timeinfo));
  String Uptime = (String(millis()/1000/60/60))+":";
  Uptime += (((millis()/1000/60%60)<10)?"0"+String(millis()/1000/60%60):String(millis()/1000/60%60))+":";
  Uptime += ((millis()/1000%60)<10)?"0"+String(millis()/1000%60):String(millis()/1000%60);
  webpage += "<p>Uptime: " + Uptime + "</p>";
  */
  append_page_footer();
  Webserver.send(200, "text/html", webpage); // Send a response to the client to enter their inputs, if needed, Enter=defaults
  //webpage = "";

}
