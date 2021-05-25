// Copyright (c) 2021 SQ9MDD Rysiek Labus
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <LittleFS.h>

#include <html.h>
#include <settings_wifi.h>
#include <settings_domo.h>
#include <settings_aprs.h>
#include <settings.h>
#include <version.h>

#define AP_SSID "SINUX WX AP"
#define AP_PASS "sinux2021"

extern ESP8266WebServer server;

void getSettings();
void getJSON();
void handle_Index();
void restServerRouting();
void handleNotFound();
void connect_to_wifi();