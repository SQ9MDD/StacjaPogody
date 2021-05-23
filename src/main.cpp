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
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <EEPROM.h>
#include <LittleFS.h>
#include <taskWebServer.h>

const byte wind_sensor = D5;
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

unsigned long last_read = 0;
unsigned long last_rpm_read = 0;
String gardner_name = "SINUX WS.0.3";
String wifi_config_file = "/wifi_conf.txt";
String config_file = "/config.txt";
String wifi_ssid = "";
String wifi_pass = "";
String www_pass = "sinux2021";
boolean spiffsActive = false;
boolean sensor_ok = true;
float sensor_temperature = 0.0;
float sensor_humidity = 0.0;
float sensor_dewpoint = 0.0;
float sensor_windchill = 0.0;
float sensor_baro = 0.0;
int above_sea_lvl = 0;


Adafruit_BME280 bme;              // ESP8266 connect 

void read_global(){
  if (LittleFS.begin()){
      spiffsActive = true;
  } else {
      Serial.println("Unable to activate SPIFFS");
  }
  File file = LittleFS.open(config_file,"r");
  if(!file){
    Serial.println("error opening file");
  }else{
    String s;
    int line = 0;
    while (file.position()<file.size()){
      s = file.readStringUntil('\n');
      s.trim();
      if(line == 0){  // nazwa
        gardner_name = s.c_str();
      } 
      if(line == 1){  // wysokosc
        above_sea_lvl = s.toInt();
      }   
      if(line == 2){  // www password
        www_pass = s.c_str();
      }    
      if(line == 3){  // anemometr diameter
        diameter_mm = s.toInt();
      }  
      if(line == 4){  // anemometr calibration factor
        kalibracja = s.toFloat();
      }                  
      line++;
    }
  file.close();    
  }
}

void read_wifi_spiffs(){
  if (LittleFS.begin()){
      spiffsActive = true;
  } else {
      Serial.println("Unable to activate SPIFFS");
  }
  File file = LittleFS.open(wifi_config_file,"r");
  if(!file){
    Serial.println("error opening file");
  }
    String s;
    int line = 0;
    while (file.position()<file.size()){
      s = file.readStringUntil('\n');
      s.trim();
      if(line == 0){  //bot api
        wifi_ssid = s.c_str();
      }
      if(line == 1){  //chat id
        wifi_pass = s.c_str();
      }    
      line++;
    }
  file.close();
}

double dewPointFast(double celsius, double humidity){
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity*0.01);
  double Td = (b * temp) / (a - temp);
  return Td;
}

void read_bme(){
  float hPa_offset = float(above_sea_lvl) * 0.10933;
  sensor_temperature = ((sensor_temperature * 9 ) + bme.readTemperature()) / 10;
  sensor_humidity = ((sensor_humidity * 9) + bme.readHumidity()) / 10;
  sensor_dewpoint = dewPointFast(sensor_temperature,sensor_humidity);
  sensor_baro = bme.readPressure() / 100.0F + hPa_offset;
}

void IRAM_ATTR wind_tick(){
  interruptCounter++;
}

void setup() {
  pinMode(wind_sensor,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(wind_sensor), wind_tick, FALLING);

  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n");
  Serial.println("read config...");   
  read_wifi_spiffs();
  read_global();
  delay(2000);
  Serial.println("booting...");
  connect_to_wifi();
  restServerRouting();
  server.onNotFound(handleNotFound);
  server.begin();
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

  // test for BME280 sensor
  if (!bme.begin(0x76)){
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    sensor_ok = false;
  } 

  Serial.println("booting done, enjoy\n"); 
  float hPa_offset = float(above_sea_lvl) * 0.10933;
  sensor_temperature = bme.readTemperature();
  sensor_humidity = bme.readHumidity();
  sensor_dewpoint = dewPointFast(sensor_temperature,sensor_humidity);
  sensor_baro = bme.readPressure() / 100.0F + hPa_offset;  
  Serial.println("www username is: root");
  Serial.println("www password is: " + String(www_pass));
  Serial.println("system name: " + String(gardner_name));
  Serial.println("baro montage lvl: " + String(above_sea_lvl));
  Serial.println("anemometer diameter: " + String(diameter_mm));
  Serial.println("anemometer calibration: " + String(kalibracja) + "\n");
  Serial.println("millis;temp;humidity;dewpoint;barometer;wind;gust");
}

void loop(){
  ArduinoOTA.handle();
  server.handleClient();

  if(millis() - last_rpm_read > 10000){
    last_rpm_read = millis();
    rpm = (interruptCounter * 6) / 4;
    interruptCounter = 0;
    // obliczenie sredniej predkosci wiatru uśrednianie ostatnich 6-ciu pomiarów = średnia z minuty
    rpm_avg = (rpm_avg * 5 + rpm) / 6;
    ms = (diameter_mm * 3.14 * rpm_avg) / 1000 / 60;
    ms = ms * kalibracja; 

    // obliczenie predkosci max
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
    if(sensor_temperature < 15.0 && ms > 1.34){
      float MPH = ms * 2.23693629;
      float TempF = (sensor_temperature * 9) / 5 + 32;
      float windchillF = 35.74 + 0.6215 * TempF - 35.75 * pow(MPH, 0.16) + 0.4275 * TempF * pow(MPH, 0.16);
      sensor_windchill = (windchillF - 32) * 5 / 9;      
    }else{
      sensor_windchill = sensor_temperature;
    }
  }
 
  if(millis() - last_read > 5000){
    read_bme();
    last_read = millis();
    Serial.println(String(millis()) + ";" + String(sensor_temperature,1) + ";" + String(sensor_humidity,0) + ";" + String(sensor_dewpoint,1) + ";" + String(sensor_baro,1) + ";" + String(ms) + ";" + String(ms_max));
  }
}