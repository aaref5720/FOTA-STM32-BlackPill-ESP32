#ifndef MQTT_H
#define MQTT_H

/*************************include *************************/
#include <Arduino.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include "Connect_to_Wifi.h"
#include "secrets.h"

/**************defines**********************************/
/***Adafruit.io settings ***/
#define WIFI_SSID       "your_wifi_ssid"
#define WIFI_PASSWORD   "your_wifi_password"
#define AIO_USERNAME    "Your_Username"
#define AIO_KEY         "your_adafruit_key_here"

#define MAX_MQTT_RETRIES    5
extern Adafruit_MQTT_Publish BootloaderReply;
extern Adafruit_MQTT_Subscribe BootloaderCommand;

/**** external  MQTT objects ****/
extern Adafruit_MQTT_Publish BL_Reply;
extern Adafruit_MQTT_Subscribe BL_Command;
extern Adafruit_MQTT_Client mqtt;

void MQTT_connect();

#endif // MQTT_H
