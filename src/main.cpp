// Copyright (c) 2021 SQ9MDD Rysiek Labus
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
//#include <WiFiClientSecureBearSSL.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
//#include <BME280I2C.h>
#include <Wire.h>
//#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <EEPROM.h>
#include <LittleFS.h>
#include <taskWebServer.h>

boolean first_reading = true;
const byte wind_sensor = D5;
const byte bme_pwr = D6;
const byte dire_sensor = A0;
volatile byte interruptCounter = 0;
float ms;
float ms_max;
int rpm = 0;
int rpm_max = 0;
int rpm_avg = 0;
int rpm_max_arr[30];
int rpm_max_pointer = 0;
float diameter_mm = 216;    // srednica anemometru w mm
float kalibracja = 2.9;     // kalibracja miernika 
int direction_raw = 0;
int direction = 0;
int direction_abs = 0;
int dir_calibration = 0;
unsigned long last_read = 0;
unsigned long last_rpm_read = 0;
unsigned long last_direction_read = 0;

// zmienne integracji z domoticzem
int domoti_IP_1 = 192;
int domoti_IP_2 = 168;
int domoti_IP_3 = 0;
int domoti_IP_4 = 1;
int domoti_PORT = 80;
int idx_temp_rh_baro_sensor = 1;
int idx_wind_sensor = 2;
int send_interval = 5;
boolean domoti_on = false;
unsigned long domoticz_interval = 0;
unsigned long last_data_send = 0;

//zmienne integracji z APRS / CWOP
String aprs_user = "FW0000";
String aprs_pass = "-1";
String aprs_lat = "5200.01N";
String aprs_lon = "02055.01E";
String aprs_addr = "cwop1.aprs.net";
int aprs_port = 14580;
int aprs_send_interval = 10;
boolean aprs_on = false;
unsigned long aprs_interval = 0;
unsigned long aprs_last_data_send = 0;

String gardner_name = "Stacja Pogody";
String config_file = "/config.txt";
String wifi_config_file = "/wifi_conf.txt";
String domoti_config_file = "/domoti_conf.txt";
String aprs_config_file = "/aprs_conf.txt";
String wifi_ssid = "";
String wifi_pass = "";
String www_pass = "root";
boolean spiffsActive = false;
boolean sensor_ok = true;
float sensor_temperature = 0.0;
float sensor_humidity = 0.0;
float sensor_dewpoint = 0.0;
float sensor_windchill = 0.0;
float sensor_baro = 0.0;
int above_sea_lvl = 0;

Adafruit_BME280 bme;              // ESP8266 connect 

void read_global_config(){
  if (LittleFS.begin()){
      spiffsActive = true;
  } else {
      Serial.println("Unable to activate SPIFFS");
  }
  File file = LittleFS.open(config_file,"r");
  if(!file){
    Serial.println("error opening global config...");
  }else{
    String s;
    int line = 0;
    while (file.position()<file.size()){
      s = file.readStringUntil('\n');
      s.trim();
      if(line == 0){ gardner_name = s.c_str(); } 
      if(line == 1){ above_sea_lvl = s.toInt(); }   
      if(line == 2){ www_pass = s.c_str(); }    
      if(line == 3){ diameter_mm = s.toInt(); }  
      if(line == 4){ kalibracja = s.toFloat(); }   
      if(line == 5){ dir_calibration = s.toInt(); }                       
      line++;
    }
  file.close();
  Serial.println("read global config ok...");     
  }
}

void read_wifi_config(){
  if (LittleFS.begin()){
      spiffsActive = true;
  } else {
      Serial.println("Unable to activate SPIFFS");
  }
  File file = LittleFS.open(wifi_config_file,"r");
  if(!file){
    Serial.println("error opening WiFi config...");
  }
    String s;
    int line = 0;
    while (file.position()<file.size()){
      s = file.readStringUntil('\n');
      s.trim();
      if(line == 0){ wifi_ssid = s.c_str(); }
      if(line == 1){ wifi_pass = s.c_str(); }    
      line++;
    }
  file.close();
  Serial.println("read WiFi config ok..."); 
}

void read_domoti_config(){
  if (LittleFS.begin()){
      spiffsActive = true;
  } else {
      Serial.println("Unable to activate SPIFFS");
  }
  File file = LittleFS.open(domoti_config_file,"r");
  if(!file){
    Serial.println("error opening domoticz config...");
  }  
    String s;
    int line = 0;
    while (file.position()<file.size()){
      s = file.readStringUntil('\n');
      s.trim();
      if(line == 0){ domoti_IP_1 = s.toInt(); }
      if(line == 1){ domoti_IP_2 = s.toInt(); }    
      if(line == 2){ domoti_IP_3 = s.toInt(); } 
      if(line == 3){ domoti_IP_4 = s.toInt(); }
      if(line == 4){ domoti_PORT = s.toInt(); } 
      if(line == 5){ send_interval = s.toInt(); } 
      if(line == 6){ domoti_on = s.toInt(); } 
      if(line == 7){ idx_temp_rh_baro_sensor = s.toInt(); } 
      if(line == 8){ idx_wind_sensor = s.toInt(); } 
      line++;
    }
  file.close();
  Serial.println("read domoticz config ok...");   
}

void read_aprs_config(){
  if (LittleFS.begin()){
      spiffsActive = true;
  } else {
      Serial.println("Unable to activate SPIFFS");
  }
  File file = LittleFS.open(aprs_config_file,"r");
  if(!file){
    Serial.println("error opening aprs config...");
  }  
    String s;
    int line = 0;
    while (file.position()<file.size()){
      s = file.readStringUntil('\n');
      s.trim();
      if(line == 0){ aprs_addr = s.c_str(); }
      if(line == 1){ aprs_port = s.toInt(); }    
      if(line == 2){ aprs_send_interval = s.toInt(); } 
      if(line == 3){ aprs_on = s.toInt(); }
      if(line == 4){ aprs_user = s.c_str(); } 
      if(line == 5){ aprs_pass = s.c_str(); } 
      if(line == 6){ aprs_lat = s.c_str(); } 
      if(line == 7){ aprs_lon = s.c_str(); } 
      line++;
    }
  file.close();  
  Serial.println("read aprs config ok..."); 
}

// type_sensor enum: 
// 1-temperature humidity baro, 
// 2- wind speed nd direction
void send_domoticz(int type_sensor, int idx){
  String url = "http://" + String(domoti_IP_1) + "." + String(domoti_IP_2) + "." + String(domoti_IP_3) + "." + String(domoti_IP_4) + ":" +String(domoti_PORT) + "/json.htm";
  int hum_stat = 0;
  int baro_stat = 0;
  if(type_sensor == 1){
    if(sensor_humidity < 35.0) hum_stat = 2; 
    if(sensor_humidity >= 35.0 && sensor_humidity < 65.0) hum_stat = 1;
    if(sensor_humidity >= 65.0) hum_stat = 3;
    if(sensor_baro < 1000) baro_stat  = 4;
    if(sensor_baro >= 1000 && sensor_baro <1010) baro_stat  = 3;
    if(sensor_baro >= 1010 && sensor_baro <1020) baro_stat  = 2;
    if(sensor_baro >= 1020) baro_stat  = 1;
    url +=  "?type=command&param=udevice&idx=" + String(idx) + "&nvalue=0&svalue=" + String(sensor_temperature,1) + ";" + String(sensor_humidity) + ";" + String(hum_stat) + ";" + String(sensor_baro) + ";" + String(baro_stat);
  }
  if(type_sensor == 2){
    String direction_stat = "N";
    if(direction < 45 || direction > 315) direction_stat = "N";
    if(direction >= 45 && direction < 135) direction_stat = "E";
    if(direction >= 135 && direction < 225) direction_stat = "S";
    if(direction >= 225 && direction < 315) direction_stat = "W";
    url += "?type=command&param=udevice&idx=" + String(idx) + "&nvalue=0&svalue=" + String(direction) + ";" + direction_stat + ";" + String(ms*10) + ";" + String(ms_max*10) + ";" + String(sensor_temperature,1) + ";" + String(sensor_windchill) + ";" ;
  } 
  WiFiClient client;
  HTTPClient http;
  http.begin(client,url);
  http.GET();
  // dopisac obslugę błędów
  http.end();
  Serial.println(url);
}

String prepare_direction(int input){
  char wind_dir_str[4];
  sprintf(wind_dir_str,"%03d", input);
  return(wind_dir_str);
}

String prepare_wind(int input){
  int wind_mph = input * 3600.0 / 1609.0;
  char wind_speed_str[4];
  sprintf(wind_speed_str,"%03d", wind_mph);
  return(wind_speed_str);
}

String prepare_temp(float input){
  float TempF = (input * 9) / 5 + 32; 
  if(TempF < 100){
    return("0" + String(TempF,0));
  }else{
    return(String(TempF,0));
  }
}

String prepare_hum(float input){
  if(input == 100){
    return("00");
  }else{
    return(String(input,0));
  }
}

void send_aprs(){
  // priv var   
  String login = "user " + aprs_user + " pass " + aprs_pass + " vers SINUX." + String(VERSION_SHORT);
  String aprs_frame = aprs_user + ">APRS,TCPIP*:=" + aprs_lat + "/" + aprs_lon + "_" +  prepare_direction(direction) + "/" + prepare_wind(ms) + "g" + prepare_wind(ms_max) + "t" + prepare_temp(sensor_temperature) + "h" + prepare_hum(sensor_humidity) + "b" + String((sensor_baro * 10),0) + "oDtz";
  // lets play some network
  WiFiClient client;
  int retry = 10;
  while(!client.connect(aprs_addr, aprs_port) && (retry > 0)){
    delay(100);
    retry--;
  }
  if (!client.connected()){
      Serial.println("connection failed");
      client.stop();
      return;
  }else{
      client.println(login);
      Serial.println(login);
      delay(3000); //as reccomended, 3" between login and sends packet
      client.println(aprs_frame);
      Serial.println(aprs_frame);
      client.stop();
  }    
}

double dewPointFast(double celsius, double humidity){
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity*0.01);
  double Td = (b * temp) / (a - temp);
  return Td;
}

double heatIndex(double tempC, double humidity){
  double TempF = (tempC * 9) / 5 + 32;
  double c1 = -42.38, c2 = 2.049, c3 = 10.14, c4 = -0.2248, c5= -6.838e-3, c6=-5.482e-2, c7=1.228e-3, c8=8.528e-4, c9=-1.99e-6 ; 
  double T = TempF;
  double R = humidity;
  double T2 = T*T;
  double R2 = R*R;
  double TR = T*R;
  double rv = c1 + c2*T + c3*R + c4*T*R + c5*T2 + c6*R2 + c7*T*TR + c8*TR*R + c9*T2*R2;
  rv = (rv - 32) * 5 / 9;
  return(rv);
}

double windchill(double tempC, double ms){
  float MPH = ms * 2.23693629;
  float TempF = (tempC * 9) / 5 + 32;
  float windchillF = 35.74 + 0.6215 * TempF - 35.75 * pow(MPH, 0.16) + 0.4275 * TempF * pow(MPH, 0.16);
  sensor_windchill = (windchillF - 32) * 5 / 9;   
  return(sensor_windchill);
}

void read_bme(){
  digitalWrite(bme_pwr,HIGH);
  delay(50);
  // test for BME280 sensor
  if (!bme.begin(0x76)){
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    sensor_ok = false;
  }
  delay(50);
  float hPa_offset = float(above_sea_lvl) * 0.10933;

  if(first_reading){
    sensor_temperature = bme.readTemperature();
    sensor_humidity = bme.readHumidity();
    sensor_dewpoint = dewPointFast(sensor_temperature,sensor_humidity);
    sensor_baro = bme.readPressure() / 100.0F + hPa_offset; 
    first_reading = false;    
  } 

  if(String(sensor_humidity,0) == "nan" || String(sensor_temperature,1) == "nan" || String(sensor_dewpoint,1) == "nan"){
    Serial.println("have to reboot");
    delay(1000);
    ESP.restart();
  }

  sensor_temperature = ((sensor_temperature * 9 ) + bme.readTemperature()) / 10;
  sensor_humidity = ((sensor_humidity * 9) + bme.readHumidity()) / 10;
  sensor_dewpoint = dewPointFast(sensor_temperature,sensor_humidity);
  sensor_baro = bme.readPressure() / 100.0F + hPa_offset;
  
  //Serial.println(bme.readTemperature());
  digitalWrite(bme_pwr,LOW);
}

// licznik obrotów prędkościomierza wiatru
void IRAM_ATTR wind_tick(){
  interruptCounter++;
}

int calculate_abs_degree(int mesure, int min_sensor, int max_sensor, int rev_sensor){
  int direction_abs = 0;
  if(mesure <= min_sensor){
    return(0);
  }
  if(rev_sensor == 1){
    direction_abs = map(mesure,min_sensor,max_sensor,315,0);
  }else{
    direction_abs = map(mesure,min_sensor,max_sensor,0,315);
  }
  return(direction_abs);
}

int calculate_rel_degree(int direction_abs, int calibration_offset){
  int direction_rel = 0;
  direction_rel = direction_abs + calibration_offset;
  if(direction_rel > 360){
    direction_rel = direction_rel - 360;
  }
  return(direction_rel);  
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n");
  Serial.println("booting...");
  pinMode(wind_sensor,INPUT_PULLUP);
  pinMode(dire_sensor,INPUT);
  pinMode(bme_pwr,OUTPUT);
  digitalWrite(bme_pwr,LOW);
  attachInterrupt(digitalPinToInterrupt(wind_sensor), wind_tick, FALLING);
  delay(1000); 
  
  Serial.println("read config files...");   
  read_global_config();
  read_wifi_config();
  read_domoti_config();
  read_aprs_config();
  delay(2000);

  Serial.println("activating network..."); 
  connect_to_wifi();
  restServerRouting();
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("activating OTA..."); 
  ArduinoOTA.setPassword(INI_UPLOAD_PASS);
  ArduinoOTA.onStart([](){
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH){
      type = "sketch";
    }else{
      type = "filesystem";
    }
	});
  ArduinoOTA.begin();
  Serial.println("booting done, enjoy\n"); 
  Serial.println("www username is: root");
  Serial.println("www password is: " + String(www_pass));
  Serial.println("system name: " + String(gardner_name) + "\nReady to play...\n");
  //Serial.println("millis;temp;humidity;dewpoint;barometer;wind;gust");
}

void loop(){
  ArduinoOTA.handle();
  server.handleClient();

  if(millis() - last_direction_read > 500){
    last_direction_read = millis();
    int dir_raw = analogRead(dire_sensor);
    // odczytanie kierunku raw z przetwornika
    if(dir_raw > 50){
      direction_raw = (direction_raw * 5 + dir_raw) / 6;
      direction_abs = calculate_abs_degree(direction_raw, 114, 981, 0);
      direction = calculate_rel_degree(direction_abs, dir_calibration);
    }
  }
  // compute wind speed, gust and wind chill
  if(millis() - last_rpm_read > 10000){
    last_rpm_read = millis();
    rpm = (interruptCounter * 6) / 4;
    interruptCounter = 0;
    // obliczenie sredniej predkosci wiatru uśrednianie ostatnich 6-ciu pomiarów = średnia z minuty
    rpm_avg = (rpm_avg * 5 + rpm) / 6;
    ms = (diameter_mm * 3.14 * rpm_avg) / 1000 / 60;
    ms = ms * kalibracja; 
    // obliczenie predkosci max (GUST)
    // dodaj kazdy pomiar do tablicy odczytaj z tablicy wartość max, 30el w tablicy co 10sec = 5min
    rpm_max_arr[rpm_max_pointer] = rpm;
    rpm_max_pointer++;
    if(rpm_max_pointer == 30){
      rpm_max_pointer = 0;
    }
    rpm_max = 0;
    for(int i = 0; i < 30; i++){
      if(rpm_max < rpm_max_arr[i]){
        rpm_max = rpm_max_arr[i];
      }
    }
    ms_max = (diameter_mm * 3.14 * rpm_max) / 1000 / 60;
    ms_max = ms_max * kalibracja;
    // Wyliczenie temp odczuwalnej
    // ponizej 15 i przy wietrze windchill, powyzej 27C heat index
    sensor_windchill = sensor_temperature;
    if(sensor_temperature < 15.0 && ms > 1.34){
      sensor_windchill = windchill(sensor_temperature, ms);
    }else if(sensor_temperature >= 27){
      sensor_windchill = heatIndex(sensor_temperature, sensor_humidity);
    }    
  }

  // domoticz integration
  domoticz_interval = u_long(send_interval * 60 * 1000);
  if((millis() - last_data_send > domoticz_interval) && domoti_on){
    send_domoticz(1, idx_temp_rh_baro_sensor);
    send_domoticz(2, idx_wind_sensor);
    last_data_send = millis();
  }

  // aprs integration
  aprs_interval = u_long(aprs_send_interval * 60 * 1000);
  if((millis() - aprs_last_data_send > aprs_interval) && aprs_on){
    send_aprs();
    aprs_last_data_send = millis();
  }

  if(millis() - last_read > 5000){
    read_bme();
    last_read = millis();
  }

  // uggly method for bme280 failure, hope it works
  unsigned long reset_time = 24 * 60 * 60 * 1000; // 86400000
  if(millis() >= reset_time){
    ESP.restart();
  }
}