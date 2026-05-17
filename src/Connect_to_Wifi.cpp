#include "Connect_to_Wifi.h"

void connectToWiFi(String ssid, String password) {
  Serial.print("Connecting to WiFi network: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
    unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 5000) { // Wait for connection with a timeout of 5 seconds
    Serial.print(".");
    delay(100);
  }
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("Connected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println();
    Serial.println("Failed to connect to WiFi!");
  }

}