#include "Fire_Base.h"

/****************** HTTP Object *********************************/
HTTPClient http;

/***************************************************************************/

void fcsDownloadCallback(int progress)
{
    Serial.printf("Downloaded %d%%\n", progress);
}

void FireBase_Init()
{
    // GitHub does not need init
    Serial.println("GitHub FOTA Client Ready\n");
}

uint8_t FireBase_DownloadFile(std::string server_file, std::string local_file)
{
    uint8_t Download_status = DOWNLOAD_SUCCESSFULL;

    Serial.println("\nDownload file...\n");

    // server_file is ignored here because the URL is fixed on GitHub
    http.begin(STORAGE_BUCKET_ID);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("Download failed, %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return DOWNLOAD_FAILED;
    }

    // Download to local file on LittleFS — same as what Firebase was doing
    int contentLength = http.getSize();
    WiFiClient* stream = http.getStreamPtr();

    File file = LittleFS.open(local_file.c_str(), "w");
    if (!file) {
        Serial.println("Download failed, cannot open local file");
        http.end();
        return DOWNLOAD_FAILED;
    }

    uint8_t buff[2048];
    int totalWritten = 0;

    while (http.connected() && totalWritten < contentLength) {
        size_t available = stream->available();
        if (available) {
            int bytesRead = stream->readBytes(buff, min(available, sizeof(buff)));
            file.write(buff, bytesRead);
            totalWritten += bytesRead;

            // same callback as Firebase
            fcsDownloadCallback((totalWritten * 100) / contentLength);
        }
    }

    file.close();
    http.end();

    if (totalWritten != contentLength) {
        Serial.printf("Download failed, written %d of %d\n", totalWritten, contentLength);
        Download_status = DOWNLOAD_FAILED;
    } else {
        Serial.println("Download completed\n");
    }

    return Download_status;
}