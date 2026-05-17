#ifndef FIRE_BASE_H
#define FIRE_BASE_H

/****************** Includes *********************************/
#include <HTTPClient.h>
#include <LittleFS.h>

/****************** Macros *********************************/
#define DOWNLOAD_SUCCESSFULL 0x01
#define DOWNLOAD_FAILED      0x00

#define STORAGE_BUCKET_ID "https://github.com/aaref5720/fota-firmware/releases/download/v1.0.0/FOTA_Application.bin"

/****************** Global Functions Prototypes *********************************/
void FireBase_Init();
uint8_t FireBase_DownloadFile(std::string server_file, std::string local_file);
void fcsDownloadCallback(int progress);

#endif