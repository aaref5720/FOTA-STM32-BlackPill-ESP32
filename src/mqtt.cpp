#include "mqtt.h"

static WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish BL_Reply = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/bl-reply");
Adafruit_MQTT_Subscribe BL_Command = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/bl-command");

Adafruit_MQTT_Publish BootloaderReply = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/bl-reply");
Adafruit_MQTT_Subscribe BootloaderCommand = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/bl-command", MQTT_QOS_1);

static unsigned long ping_clk = 0;

void MQTT_connect() {

  if (mqtt.connected()) return;

  unsigned long start = millis();
  long attempts = 0;

  while (attempts < MAX_MQTT_RETRIES) {
    attempts++;

    Serial.print("MQTT Connect Attempt #");
    Serial.print(attempts);
    Serial.print(" ... ");

    int8_t ret = mqtt.connect();

    if (ret == 0) {
      Serial.print("Connected in ");
      Serial.print(millis() - start);
      Serial.println(" ms");
      ping_clk = millis();
      return;
    }

    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying in 5 seconds...");
    mqtt.disconnect();
    delay(5000);
  }

  Serial.println("ERROR: Maximum MQTT connect attempts exceeded.");
  while (1) delay(10);
}