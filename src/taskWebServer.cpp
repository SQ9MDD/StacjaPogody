// Copyright (c) 2021 SQ9MDD Rysiek Labus
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "taskWebServer.h"

ESP8266WebServer server(80);
extern float diameter_mm;
extern float kalibracja;
extern String gardner_name;
extern String config_file;
extern String wifi_config_file;
extern String domoti_config_file;
extern String aprs_config_file;
extern String wifi_ssid;
extern String wifi_pass;
extern String www_pass;
extern boolean spiffsActive;
extern boolean sensor_ok;
extern float sensor_temperature;
extern float sensor_humidity;
extern float sensor_dewpoint;
extern float sensor_baro;
extern int sensor_uv_raw;
extern float sensor_uv_voltage;
extern float sensor_uv_index;
extern int above_sea_lvl;
extern int domoti_IP_1;
extern int domoti_IP_2;
extern int domoti_IP_3;
extern int domoti_IP_4;
extern int domoti_PORT;
extern int send_interval;
extern boolean domoti_on;
extern int idx_temp_rh_baro_sensor;
extern String aprs_user;
extern String aprs_pass;
extern String aprs_lat;
extern String aprs_lon;
extern String aprs_addr;
extern int aprs_port;
extern int aprs_send_interval;
extern boolean aprs_on;

String humidity_status(int humidity){
  if(humidity < 35) return "Dry";
  if(humidity < 65) return "Comfortable";
  return "Wet";
}

bool send_domoticz_getdevices_by_rid(){
  if(server.arg("type") != "devices"){
    return false;
  }

  String rid = server.arg("rid");
  if(rid != "24" && rid != "25" && rid != "26" && rid != "27" && rid != "28"){
    return false;
  }
  String result;

  if(rid == "24"){
    result = "{\"idx\":\"24\",\"Name\":\"Temperature\",\"Type\":\"Temp\",\"Temp\":" + String(sensor_temperature,1) + ",\"Data\":\"" + String(sensor_temperature,1) + " C\"}";
  }else if(rid == "25"){
    int hum = int(sensor_humidity + 0.5);
    result = "{\"idx\":\"25\",\"Name\":\"Humidity\",\"Type\":\"Humidity\",\"Humidity\":" + String(hum) + ",\"HumidityStatus\":\"" + humidity_status(hum) + "\",\"Data\":\"" + String(hum) + " %\"}";
  }else if(rid == "26"){
    result = "{\"idx\":\"26\",\"Name\":\"Pressure\",\"Type\":\"Barometer\",\"Barometer\":" + String(sensor_baro,1) + ",\"Data\":\"" + String(sensor_baro,1) + " hPa\"}";
  }else if(rid == "27"){
    result = "{\"idx\":\"27\",\"Name\":\"Dew Point\",\"Type\":\"Temp\",\"DewPoint\":" + String(sensor_dewpoint,1) + ",\"Data\":\"" + String(sensor_dewpoint,1) + " C\"}";
  }else if(rid == "28"){
    result = "{\"idx\":\"28\",\"Name\":\"UV Index\",\"Type\":\"UV\",\"UVI\":" + String(sensor_uv_index,1) + ",\"Data\":\"" + String(sensor_uv_index,1) + " UVI\"}";
  }

  String buf = "{\"status\":\"OK\",\"title\":\"Devices\",\"ActTime\":" + String(millis()) + ",\"ServerTime\":\"\",\"result\":[" + result + "]}";
  server.send(200, F("application/json"), buf);
  return true;
}
void getJSON(){
  String sensor_status;
  if(sensor_ok){
    if(sensor_baro < 1010.0) sensor_status += "Pochmurnie";
    if(sensor_baro > 1020.0) sensor_status += "Slonecznie";
    if(sensor_baro >= 1010.0 && sensor_baro <= 1020.0) sensor_status += "Przejsciowe zachmurzenie";
    if(sensor_humidity > 85 && sensor_temperature > 3) sensor_status += ", mozliwe opady deszczu";
    if(sensor_humidity > 85 && sensor_temperature <= 3) sensor_status += ", mozliwe opady sniegu";
    if(sensor_temperature < 10) sensor_status += ", zimno.";
    if(sensor_temperature >= 10 && sensor_temperature <= 15) sensor_status += ", chlodno.";
    if(sensor_temperature > 15 && sensor_temperature <= 18) sensor_status += ", temperatura umiarkowana.";
    if(sensor_temperature > 18 && sensor_temperature <= 22) sensor_status += ", cieplo.";
    if(sensor_temperature > 22) sensor_status += ", upalnie.";
  }else{
    sensor_status = "Uszkodzenie czujnika";
  }

  if(send_domoticz_getdevices_by_rid()){
    return;
  }

  if(server.arg("type") == "devices" && server.arg("rid")=="5"){
    String buf = "{\"ActiTime\": ";
    buf += millis();
    buf += ", \"result\":[{ ";
    buf += "\"Temp\": " + String(sensor_temperature);
    buf += ", \"Humidity\": " + String(sensor_humidity);
    buf += ", \"DewPoint\": " + String(sensor_dewpoint);
    buf += ", \"Barometer\": " + String(sensor_baro);
    buf += ", \"uv_index\": " + String(sensor_uv_index,1);
    buf += ", \"uv_voltage\": " + String(sensor_uv_voltage,3);
    buf += ", \"uv_raw\": " + String(sensor_uv_raw);
    buf += "}]}";
    server.send(200, F("application/json"), buf);
  }else if(server.arg("type") == "devices" && server.arg("rid")=="1"){
    String buf = "{\"ActiTime\": ";
    buf += millis();
    buf += ", \"Temp\": \"" + String(sensor_temperature,1) + "\"";
    buf += ", \"Humidity\": \"" + String(sensor_humidity,0) + "\"";
    buf += ", \"DewPoint\": \"" + String(sensor_dewpoint,1) + "\"";
    buf += ", \"Barometer\": \"" + String(sensor_baro,1) + "\"";
    buf += ", \"uv_index\": \"" + String(sensor_uv_index,1) + "\"";
    buf += ", \"uv_voltage\": \"" + String(sensor_uv_voltage,3) + "\"";
    buf += ", \"uv_raw\": \"" + String(sensor_uv_raw) + "\"";
    buf += ", \"job_status\": \"" + sensor_status + "\"";
    buf += "}";
    server.send(200, F("application/json"), buf);
  }else if(server.arg("type") == "devices" && server.arg("rid")=="2"){
    if(!server.authenticate("root", www_pass.c_str())) return server.requestAuthentication(DIGEST_AUTH, "login required for user root", "Authentication Failed");
    String buf = "{\"time\": ";
    buf += millis();
    buf += ", \"gardner_name\": \"" + gardner_name + "\"";
    buf += ", \"above_sea_lvl\": \"" + String(above_sea_lvl) + "\"";
    buf += ", \"www_pass\": \"" + String(www_pass) + "\"";
    buf += ", \"diameter_mm\": \"" + String(diameter_mm,0) + "\"";
    buf += ", \"kalibracja\": \"" + String(kalibracja,1) + "\"";
    buf += "}";
    server.send(200, F("application/json"), buf);
  }else if(server.arg("type") == "devices" && server.arg("rid")=="3"){
    if(!server.authenticate("root", www_pass.c_str())) return server.requestAuthentication(DIGEST_AUTH, "login required for user root", "Authentication Failed");
    String buf = "{\"time\": ";
    buf += millis();
    buf += ", \"wifi_ssid\": \"" + wifi_ssid + "\"";
    buf += ", \"wifi_pass\": \"" + wifi_pass + "\"";
    buf += "}";
    server.send(200, F("application/json"), buf);
  }else if(server.arg("type") == "devices" && server.arg("rid")=="6"){
    if(!server.authenticate("root", www_pass.c_str())) return server.requestAuthentication(DIGEST_AUTH, "login required for user root", "Authentication Failed");
    String buf = "{\"time\": ";
    buf += millis();
    buf += ", \"domoti_IP_1\": \"" + String(domoti_IP_1) + "\"";
    buf += ", \"domoti_IP_2\": \"" + String(domoti_IP_2) + "\"";
    buf += ", \"domoti_IP_3\": \"" + String(domoti_IP_3) + "\"";
    buf += ", \"domoti_IP_4\": \"" + String(domoti_IP_4) + "\"";
    buf += ", \"domoti_PORT\": \"" + String(domoti_PORT) + "\"";
    buf += ", \"send_interval\": \"" + String(send_interval) + "\"";
    buf += ", \"idx_temp_rh_baro_sensor\": \"" + String(idx_temp_rh_baro_sensor) + "\"";
    buf += ", \"domoti_on\": ";
    buf += domoti_on;
    buf += "}";
    server.send(200, F("application/json"), buf);
  }else if(server.arg("type") == "devices" && server.arg("rid")=="7"){
    String buf = "{\"time\": ";
    buf += millis();
    buf += ", \"aprs_addr\": \"" + aprs_addr + "\"";
    buf += ", \"aprs_port\": ";
    buf += aprs_port;
    buf += ", \"aprs_send_interval\": ";
    buf += aprs_send_interval;
    buf += ", \"aprs_on\": ";
    buf += aprs_on;
    buf += ", \"aprs_user\": \"" + aprs_user + "\"";
    buf += ", \"aprs_pass\": \"" + aprs_pass + "\"";
    buf += ", \"aprs_lat\": \"" + aprs_lat + "\"";
    buf += ", \"aprs_lon\": \"" + aprs_lon + "\"";
    buf += "}";
    server.send(200, F("application/json"), buf);
  }else if(server.arg("type") == "devices" && server.arg("rid")=="4"){
    String buf = "{\"time\": ";
    buf += millis();
    buf += ", \"RSSI\": \"" + String(WiFi.RSSI()) + "\"";
    buf += "}";
    server.send(200, F("application/json"), buf);
  } else {
    String message = "{\"time\": ";
    message += millis();
    message += ", \"gardner_name\": \"" + gardner_name + "\"";
    message += ", \"title\": \"" + gardner_name + "\"";
    message += ", \"version\": \"" + String(VERSION_SHORT) + "\"";
    message += ", \"job_status\": \"" + sensor_status + "\"";
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

void save_domo(){
  if(!server.authenticate("root", www_pass.c_str())) return server.requestAuthentication(DIGEST_AUTH, "login required for user root", "Authentication Failed");
  domoti_IP_1 = server.arg("domoti_IP_1").toInt();
  domoti_IP_2 = server.arg("domoti_IP_2").toInt();
  domoti_IP_3 = server.arg("domoti_IP_3").toInt();
  domoti_IP_4 = server.arg("domoti_IP_4").toInt();
  domoti_PORT = server.arg("domoti_PORT").toInt();
  send_interval = server.arg("send_interval").toInt();
  domoti_on = server.arg("domoti_on").toInt();
  idx_temp_rh_baro_sensor = server.arg("idx_temp_rh_baro_sensor").toInt();
  if (LittleFS.begin()){
      spiffsActive = true;
  } else {
      Serial.println("Unable to activate SPIFFS");
  }
  File file = LittleFS.open(domoti_config_file,"w");
  file.print(String(domoti_IP_1) + "\n" + String(domoti_IP_2) + "\n" + String(domoti_IP_3) + "\n" + String(domoti_IP_4) + "\n" + String(domoti_PORT) + "\n" + String(send_interval) + "\n" + String(domoti_on) + "\n" + String(idx_temp_rh_baro_sensor) + "\n");
  file.close();
  server.send(200, F("text/html"), "<html><head><meta http-equiv=\"refresh\" content=\"1; url=/set_domo\"></head><body><center><br><br><br><b>OK</body></html>");
  delay(2000);
}

void save_aprs(){
  if(!server.authenticate("root", www_pass.c_str())) return server.requestAuthentication(DIGEST_AUTH, "login required for user root", "Authentication Failed");
  aprs_addr = server.arg("aprs_addr");
  aprs_port = server.arg("aprs_port").toInt();
  aprs_send_interval = server.arg("aprs_send_interval").toInt();
  aprs_on = server.arg("aprs_on").toInt();
  aprs_user = server.arg("aprs_user");
  aprs_pass = server.arg("aprs_pass");
  aprs_lat = server.arg("aprs_lat");
  aprs_lon = server.arg("aprs_lon");
  if (LittleFS.begin()){
      spiffsActive = true;
  } else {
      Serial.println("Unable to activate SPIFFS");
  }
  File file = LittleFS.open(aprs_config_file,"w");
  file.print(String(aprs_addr) + "\n" + String(aprs_port) + "\n" + String(aprs_send_interval) + "\n" + String(aprs_on) + "\n" + String(aprs_user) + "\n" + String(aprs_pass) + "\n" + String(aprs_lat) + "\n" + String(aprs_lon));
  file.close();
  server.send(200, F("text/html"), "<html><head><meta http-equiv=\"refresh\" content=\"1; url=/set_aprs\"></head><body><center><br><br><br><b>OK</body></html>");
  delay(2000);
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

void handle_Index(){
  server.send(200, "text/html", HTTP_HTML);
}

void set_domo(){
  if(!server.authenticate("root", www_pass.c_str())) return server.requestAuthentication(DIGEST_AUTH, "login required for user root", "Authentication Failed");
  server.send(200, "text/html", HTTP_DOMO);
}

void set_aprs(){
  if(!server.authenticate("root", www_pass.c_str())) return server.requestAuthentication(DIGEST_AUTH, "login required for user root", "Authentication Failed");
  server.send(200, "text/html", HTTP_APRS);
}

void handle_set_wifi(){
  if(!server.authenticate("root", www_pass.c_str())) return server.requestAuthentication(DIGEST_AUTH, "login required for user root", "Authentication Failed");
  server.send(200, "text/html", HTTP_WIFI);
}

void handle_settings(){
  if(!server.authenticate("root", www_pass.c_str())) return server.requestAuthentication(DIGEST_AUTH, "login required for user root", "Authentication Failed");
  server.send(200, "text/html", HTTP_SETTINGS);
}

void restServerRouting(){
  server.on("/", handle_Index);
  server.on("/set_domo",set_domo);
  server.on("/set_aprs",set_aprs);
  server.on("/set_wifi", handle_set_wifi);
  server.on("/settings", handle_settings);
  server.on("/json.htm", getJSON);
  server.on("/save_domo",save_domo);
  server.on("/save_aprs",save_aprs);
  server.on("/save_wifi",save_wifi);
  server.on("/save_settings",save_settings);
}

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





