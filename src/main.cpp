#include <Arduino.h>
#include "BL_Host.h"
#include "Connect_to_Wifi.h"
#include "mqtt.h"
#include <LittleFS.h>

static const int          STM32_USART1_TX_FROM_PA9  = 32;
static const int          STM32_USART1_RX_TO_PA10   = 33;
static const uint32_t     DEBUG_UART_BAUD            = 230400;
static const uint32_t     BOOTLOADER_UART_BAUD       = 230400;

void bootloaderCommandCallback(uint32_t data)
{
  memset(BL_Host_Buffer, 0, BL_HOST_BUFFER_TX_LENGTH);
  while (Serial2.available()) Serial2.read();

  switch (data)
  {
    case CBL_GET_VER_CMD:
      Serial.println("\nRequest the bootloader version");
      BL_Host_Get_Version();
      break;
    case CBL_GET_CID_CMD:
      Serial.println("\nRead the MCU chip identification number");
      BL_Host_Get_Chip_ID();
      break;
    case CBL_GET_RDP_STATUS_CMD:
      Serial.println("\nRead the FLASH Read Protection level");
      BL_Host_Get_RDP();
      break;
    case CBL_FLASH_ERASE_CMD:
      Serial.println("\nErase Application command");
      BL_Host_Erase_Application();
      break;
    case CBL_MEM_WRITE_CMD:
      Serial.println("\nFlash Application command");
      BL_Host_Flash_Application();
      break;
    case CBL_GO_TO_MAIN_APP_CMD:
      Serial.println("\nJump to Main Application command");
      BL_Host_Jump_To_Application();
      break;
    default:
      Serial.println("\nInvalid Command");
      break;
  }
}

void setup()
{
  Serial.begin(DEBUG_UART_BAUD);
  Serial2.begin(BOOTLOADER_UART_BAUD, SERIAL_8N1, STM32_USART1_TX_FROM_PA9, STM32_USART1_RX_TO_PA10);

  if (!LittleFS.begin(true)) {
    Serial.println("[LittleFS] Mount failed — formatting...");
  } else {
    Serial.println("[LittleFS] Mounted OK");
  }

  connectToWiFi(WIFI_SSID, WIFI_PASSWORD);
  BootloaderCommand.setCallback(bootloaderCommandCallback);
  mqtt.subscribe(&BootloaderCommand);
  FireBase_Init();
}

void loop()
{
  MQTT_connect();
  mqtt.processPackets(10000);
}
