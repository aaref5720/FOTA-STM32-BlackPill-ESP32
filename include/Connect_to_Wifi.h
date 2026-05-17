#ifndef CONNECT_TO_WIFI_H
#define CONNECT_TO_WIFI_H

#include <Arduino.h>
#include <WiFi.h>


#define WIFI_SSID "Vodafone-7EBC"           // Replace with your actual WiFi SSID
#define WIFI_PASSWORD "HeEs4b9YZfmcZ7MT"    // Replace with your actual WiFi password
void connectToWiFi(String ssid, String password);

#endif // CONNECT_TO_WIFI_H