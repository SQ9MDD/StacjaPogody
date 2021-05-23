// Copyright (c) 2021 SQ9MDD Rysiek Labus
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "taskWebServer.h"

ESP8266WebServer server(80);
extern float ms;
extern float ms_max;
extern float diameter_mm;
extern float kalibracja;
extern String gardner_name;
extern String config_file;
extern String wifi_config_file;
extern String wifi_ssid;
extern String wifi_pass;
extern String www_pass;
extern boolean spiffsActive;
extern boolean sensor_ok;
extern float sensor_temperature;
extern float sensor_humidity;
extern float sensor_dewpoint;
extern float sensor_windchill;
extern float sensor_baro;
extern int above_sea_lvl;

void getJSON(){
  String sensor_status;
  if(sensor_ok){
    // opis od ciśnienia
    if(sensor_baro < 1010.0) sensor_status += "Pochmurnie";
    if(sensor_baro > 1020.0) sensor_status += "Slonecznie";
    if(sensor_baro >= 1010.0 && sensor_baro <= 1020.0) sensor_status += "Przejściowe zachmurzenie";
    // opis od wilgotności
    if(sensor_humidity > 85 && sensor_temperature > 3) sensor_status += ", możliwe opady deszczu";
    if(sensor_humidity > 85 && sensor_temperature <= 3) sensor_status += ", możliwe opady śniegu";
    // opis od temperatury
    if(sensor_temperature < 10) sensor_status += ", zimno.";
    if(sensor_temperature >= 10 && sensor_temperature <= 15) sensor_status += ", chłodno.";
    if(sensor_temperature > 15 && sensor_temperature <= 18) sensor_status += ", temperatura umiarkowana.";
    if(sensor_temperature > 18 && sensor_temperature <= 22) sensor_status += ", ciepło.";
    if(sensor_temperature > 22) sensor_status += ", upalnie.";
  }else{
    sensor_status = "Uszkodzenie czujnika";
  }
  if(server.arg("type") == "devices" && server.arg("rid")=="5"){    // te dane pobiera program do integracji z BacNet (bez autentykacji)
    String buf = "{\"ActiTime\": ";
    buf += millis();   
    buf += ", \"result\":[{ ";
    buf += "\"Temp\": " + String(sensor_temperature);
    buf += ", \"Humidity\": " + String(sensor_humidity);
    buf += ", \"DewPoint\": " + String(sensor_dewpoint);
    buf += ", \"sensor_windchill\": " + String(sensor_windchill);
    buf += ", \"Barometer\": " + String(sensor_baro);
    buf += ", \"Wind\": " + String(ms); 
    buf += ", \"Gust\": " + String(ms_max);
    buf += "}]}";
    server.send(200, F("application/json"), buf);
  }else if(server.arg("type") == "devices" && server.arg("rid")=="1"){  // te dane pobiera program strona główna (bez autentykacji)
    String buf = "{\"ActiTime\": ";
    buf += millis();   
    buf += ", \"Temp\": \"" + String(sensor_temperature,1) + "\"";
    buf += ", \"Humidity\": \"" + String(sensor_humidity,0) + "\"";
    buf += ", \"DewPoint\": \"" + String(sensor_dewpoint,1) + "\"";
    buf += ", \"sensor_windchill\": \"" + String(sensor_windchill,1) + "\"";
    buf += ", \"Barometer\": \"" + String(sensor_baro,1) + "\""; 
    buf += ", \"Wind\": \"" + String(ms,1) + "\""; 
    buf += ", \"WindKMH\": \"" + String((ms * 3.6),1) + "\""; 
    buf += ", \"Gust\": \"" + String(ms_max,1) + "\""; 
    buf += ", \"GustKMH\": \"" + String((ms_max * 3.6),1) + "\""; 
    buf += ", \"job_status\": ";
    buf += "\"" + sensor_status + "\"";           
    buf += "}";
    server.send(200, F("application/json"), buf);
  }else if(server.arg("type") == "devices" && server.arg("rid")=="2"){  // formularz ustawien jednorazowo pobierany przy ladowaniu strony (autentykacja)
    if(!server.authenticate("root", www_pass.c_str())) return server.requestAuthentication(DIGEST_AUTH, "login required for user root", "Authentication Failed");
    String buf = "{\"time\": ";
    buf += millis();
    buf += ", \"gardner_name\": ";
    buf += "\"" + gardner_name + "\"";      
    buf += ", \"above_sea_lvl\": ";
    buf += "\"" + String(above_sea_lvl) + "\""; 
    buf += ", \"www_pass\": ";
    buf += "\"" + String(www_pass) + "\"";  
    buf += ", \"diameter_mm\": ";
    buf += "\"" + String(diameter_mm,0) + "\"";    
    buf += ", \"kalibracja\": ";
    buf += "\"" + String(kalibracja,1) + "\"";          
    buf += "}";
    server.send(200, F("application/json"), buf);
  }else if(server.arg("type") == "devices" && server.arg("rid")=="3"){ // formularz ustawien wifi jednorazowo pobierany przy ladowaniu strony (autentykacja)
    if(!server.authenticate("root", www_pass.c_str())) return server.requestAuthentication(DIGEST_AUTH, "login required for user root", "Authentication Failed");
    String buf = "{\"time\": ";
    buf += millis();
    buf += ", \"wifi_ssid\": ";
    buf += "\"" + wifi_ssid + "\""; 
    buf += ", \"wifi_pass\": ";
    buf += "\"" + wifi_pass + "\"";   
    buf += "}";
    server.send(200, F("application/json"), buf);      
  }else if(server.arg("type") == "devices" && server.arg("rid")=="4"){  // diagnostyka (bez autentykacji)
    String buf = "{\"time\": ";
    buf += millis();
    buf += ", \"RSSI\": ";
    buf += "\"" + String(WiFi.RSSI()) + "\"";  
    buf += "}";
    server.send(200, F("application/json"), buf);  
  }
  else{  // podstawowe informacje (bez autentykacji)
    String message = "{\"time\": ";
    message += millis(); 
    message += ", \"gardner_name\": ";
    message += "\"" + gardner_name + "\"";  
    message += ", \"title\": ";
    message += "\"" + gardner_name + "\"";  
    message += ", \"version\": ";
    message += "\"" + String(VERSION_SHORT) + "<br> build: " + String(BUILD_NUMBER) + " - " + String(BUILD_DATE) + "\"";     
    message += ", \"job_status\": ";
    message += "\"" + sensor_status + "\"";
    message += "}";
    server.send(200, F("application/json"), message);
  }
}

void save_settings(){
  if(!server.authenticate("root", www_pass.c_str())) return server.requestAuthentication(DIGEST_AUTH, "login required for user root", "Authentication Failed");
  gardner_name = server.arg("gardner_name");
  above_sea_lvl = server.arg("above_sea_lvl").toInt();
  diameter_mm = server.arg("diameter_mm").toInt();
  kalibracja = server.arg("kalibracja").toFloat();
  www_pass = server.arg("www_pass");
  if (LittleFS.begin()){
      spiffsActive = true;
  } else {
      Serial.println("Unable to activate SPIFFS");
  }  
  if(www_pass != ""){
    File file = LittleFS.open(config_file,"w");  
    file.print(gardner_name + "\n" + above_sea_lvl + "\n" + www_pass + "\n" + diameter_mm + "\n" + kalibracja + "\n");
    file.close();  
    delay(2000);
    server.send(200, F("text/html"), "<html><head><meta http-equiv=\"refresh\" content=\"1; url=/settings\"></head><body><center><br><br><br><b>OK</body></html>");
  }else{
    server.send(200, F("text/html"), "<html><head><meta http-equiv=\"refresh\" content=\"1; url=/settings\"></head><body><center><br><br><br><b>EMPTY PASSWORD</body></html>");
  }

}

void save_wifi(){
  if(!server.authenticate("root", www_pass.c_str())) return server.requestAuthentication(DIGEST_AUTH, "login required for user root", "Authentication Failed");
  wifi_ssid = server.arg("wifi_ssid");
  wifi_pass = server.arg("wifi_pass");
  if (LittleFS.begin()){
      spiffsActive = true;
  } else {
      Serial.println("Unable to activate SPIFFS");
  }  
  File file = LittleFS.open(wifi_config_file,"w"); 
  if(wifi_pass != ""){
    file.print(wifi_ssid + "\n" + wifi_pass + "\n");
    file.close();
    server.send(200, F("text/html"), "<html><head><meta http-equiv=\"refresh\" content=\"1; url=/set_wifi\"></head><body><center><br><br><br><b>OK RESTARTING</body></html>");
    delay(2000);
    ESP.restart();
  }else{
    server.send(200, F("text/html"), "<html><head><meta http-equiv=\"refresh\" content=\"1; url=/set_wifi\"></head><body><center><br><br><br><b>EMPTY PASSWORD</body></html>"); 
  }
}

// main html site
void handle_Index(){
  server.send(200, "text/html", HTTP_HTML);
}

// WiFi settings
void handle_set_wifi(){
  if(!server.authenticate("root", www_pass.c_str())) return server.requestAuthentication(DIGEST_AUTH, "login required for user root", "Authentication Failed");
  server.send(200, "text/html", HTTP_WIFI);
}

// global settings
void handle_settings(){
  if(!server.authenticate("root", www_pass.c_str())) return server.requestAuthentication(DIGEST_AUTH, "login required for user root", "Authentication Failed");
  server.send(200, "text/html", HTTP_SETTINGS);
}

// Define routing
void restServerRouting(){
  server.on("/", handle_Index);
  server.on("/set_wifi", handle_set_wifi);
  server.on("/settings", handle_settings);
  server.on("/json.htm", getJSON);
  server.on("/save_wifi",save_wifi);
  server.on("/save_settings",save_settings);
}

// Manage not found URL
void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void connect_to_wifi(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  WiFi.begin(wifi_ssid, wifi_pass);
  Serial.print("Connecting to: "); 
  Serial.println(wifi_ssid);   
  int numbers_of_try = 0;
  while( WiFi.status() != WL_CONNECTED && numbers_of_try <= 25){
      delay(500);
      Serial.print(".");  
      numbers_of_try ++;      
  }
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("\nWifi NOT Connected!");
    Serial.println("Activating AP HotSpot look for SINUX WX AP with pass: sinux2021");
    WiFi.disconnect(true);
    WiFi.softAP(AP_SSID, AP_PASS, 7);
    Serial.print("NodeMCU IP Address : ");   
    Serial.println(WiFi.softAPIP());      
  }else{
    Serial.println("\nWifi Connected Success!");
    Serial.print("NodeMCU IP Address : ");
    WiFi.hostname("SinuxWeather");
    Serial.println(WiFi.localIP());      
  }
}