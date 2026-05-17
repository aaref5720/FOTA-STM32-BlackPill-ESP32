/*******************************************************************************
 * FILE DESCRIPTION
 *******************************************************************************
 *  File:        ${file_name}
 *  Author:      Abdelrahman Mohamed
 *  Date:        ${date}
 *  Description: <Write File DESCRIPTION here>
 *******************************************************************************/

/**********************************************************************************************************************
 *  INCLUDES
 *********************************************************************************************************************/
#include "BL_Host.h"

/**********************************************************************************************************************
*  LOCAL MACROS CONSTANT\FUNCTION
*********************************************************************************************************************/

/**********************************************************************************************************************
 *  LOCAL DATA 
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *  GLOBAL DATA
 *********************************************************************************************************************/
uint8_t BL_Host_Buffer[BL_HOST_BUFFER_TX_LENGTH];
uint8_t BL_Host_Receive_Buffer[BL_HOST_BUFFER_RX_LENGTH];
/**********************************************************************************************************************
 *  LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************************/
static uint32_t Calculate_CRC32(uint8_t *data, uint8_t length);
static void Send_Data_To_Bootloader(uint8_t* data, uint8_t length);
static bool Recieve_Data_From_Bootloader(uint8_t length);
static void Print_NACK_Diagnostics(void);
static void Process_CBL_GET_VER_CMD(void);
static void Process_CBL_GET_CID_CMD(void);
static void Process_CBL_GET_RDP_CMD(void);
static void Process_CBL_FLASH_ERASE_CMD(void);
static void Process_CBL_GO_TO_MAIN_APP_CMD(void);
static bool Recieve_Data_From_Bootloader_Noprint(uint8_t length);
static uint8_t Process_CBL_MEM_WRITE_CMD(void);
/**********************************************************************************************************************
 *  LOCAL FUNCTIONS
 *********************************************************************************************************************/
static uint32_t Calculate_CRC32(uint8_t *data, uint8_t length)
{
  uint32_t CRC_Value = 0xFFFFFFFF;
    for (uint32_t DataElemindex = 0; DataElemindex < length; DataElemindex++)
    {
        CRC_Value = CRC_Value ^ data[DataElemindex];
        for(uint8_t Databitlen = 0; Databitlen < 32; Databitlen++)
        {
            if(CRC_Value & 0x80000000)
                CRC_Value = (CRC_Value << 1) ^ 0x04C11DB7;
            else
                CRC_Value = (CRC_Value << 1);
        }      
    }
    return CRC_Value;
}
static void Send_Data_To_Bootloader(uint8_t* data, uint8_t length)
{
    for(uint8_t dataindex = 0; dataindex < length; dataindex++)
    {
         Serial.print("  ");
         Serial.printf("0x%x",data[dataindex]);
         Serial.print(" ");
         Serial2.write(data[dataindex]);
         Serial2.flush();
         delayMicroseconds(500);
    }
}
static void Send_Data_To_Bootloader_Noprint(uint8_t* data, uint8_t length)
{
    for(uint8_t dataindex = 0; dataindex < length; dataindex++)
    {
         Serial2.write(data[dataindex]);
         Serial2.flush();
         delayMicroseconds(500);
    }
}
static bool Recieve_Data_From_Bootloader(uint8_t length){
    memset(BL_Host_Receive_Buffer,0,BL_HOST_BUFFER_RX_LENGTH);
    for(uint8_t dataindex = 0; dataindex < length; dataindex++)
    {
        unsigned long timeout = millis() + 3000;
        while(!Serial2.available())
        {
            if(millis() > timeout){
                Serial.println("Bootloader RX timeout!");
                return false;
            }
            yield();
        }
        BL_Host_Receive_Buffer[dataindex] = Serial2.read();
    }
    return true;
}
static bool Recieve_Data_From_Bootloader_Noprint(uint8_t length){
    memset(BL_Host_Receive_Buffer,0,BL_HOST_BUFFER_RX_LENGTH);
    for(uint8_t dataindex = 0; dataindex < length; dataindex++)
    {
        unsigned long timeout = millis() + 3000;
        while(!Serial2.available())
        {
            if(millis() > timeout){
                return false;
            }
            yield();
        }
        BL_Host_Receive_Buffer[dataindex] = Serial2.read();
    }
    return true;
}
static void Print_NACK_Diagnostics(void)
{
    delay(20);
    uint8_t diag[18] = {0};
    uint8_t count = 0;

    while (Serial2.available() && count < sizeof(diag)) {
        diag[count++] = Serial2.read();
    }

    if (count >= 17 && diag[16] == 0xEE) {
        uint32_t host_crc = (uint32_t)diag[2] |
                            ((uint32_t)diag[3] << 8) |
                            ((uint32_t)diag[4] << 16) |
                            ((uint32_t)diag[5] << 24);
        uint32_t mcu_crc = (uint32_t)diag[6] |
                           ((uint32_t)diag[7] << 8) |
                           ((uint32_t)diag[8] << 16) |
                           ((uint32_t)diag[9] << 24);

        Serial.printf("[UART NACK diag] cmd=0x%02X crc_size=%u host_crc=0x%08X mcu_crc=0x%08X raw=",
                      diag[0], diag[1], host_crc, mcu_crc);
        for (uint8_t i = 10; i < 16; i++) {
            Serial.printf("%s0x%02X", (i == 10) ? "" : " ", diag[i]);
        }
        Serial.println();
    } else if (count > 0) {
        Serial.print("[UART NACK diag] extra bytes:");
        for (uint8_t i = 0; i < count; i++) {
            Serial.printf(" 0x%02X", diag[i]);
        }
        Serial.println();
    }
}
static void Process_CBL_GET_VER_CMD(void)
{
    char BootloaderReply_str[100];
    sprintf(BootloaderReply_str,"Bootloader Vendor ID : %d \n Bootloader Version : %d . %d . %d",BL_Host_Receive_Buffer[0],BL_Host_Receive_Buffer[1],BL_Host_Receive_Buffer[2],BL_Host_Receive_Buffer[3]);
    BootloaderReply.publish(BootloaderReply_str);
    Serial.println();
    Serial.println(BootloaderReply_str);   
}
static void Process_CBL_GET_CID_CMD(void)
{
    char BootloaderReply_str[100];
    sprintf(BootloaderReply_str,"Bootloader Chip ID : 0x%x",*(uint16_t*)BL_Host_Receive_Buffer);
    BootloaderReply.publish(BootloaderReply_str);
    Serial.println();
    Serial.println(BootloaderReply_str);    
}
static void Process_CBL_GET_RDP_CMD(void)
{
   if(BL_Host_Receive_Buffer[0] == 0xAA)
   {
        BootloaderReply.publish("FLASH Protection : LEVEL 0"); 
        Serial.println("\nFLASH Protection : LEVEL 0"); 
   }
   else if(BL_Host_Receive_Buffer[0] == 0x55)
   {
        BootloaderReply.publish("FLASH Protection : LEVEL 1");  
        Serial.println("\nFLASH Protection : LEVEL 1");
   }
   else if(BL_Host_Receive_Buffer[0] == 0xCC)
   {
        BootloaderReply.publish("FLASH Protection : LEVEL 2");
        Serial.println("\nFLASH Protection : LEVEL 2");  
   }
}
static void Process_CBL_FLASH_ERASE_CMD(void)
{
    BootloaderReply.publish("Application Erased Successfully");
    Serial.println("\nApplication Erased Successfully");
}
static void Process_CBL_GO_TO_MAIN_APP_CMD(void)
{
    if(BL_Host_Receive_Buffer[0] == JUMP_SUCCESSFUL)
    {
        BootloaderReply.publish("   Jump Successful");
        Serial.println("\nJump Successful");
    }
    else if (BL_Host_Receive_Buffer[0] == JUMP_FAILED)
    {
        BootloaderReply.publish("   Jump Failed");
        Serial.println("\nJump Failed");
    }
    
}
static uint8_t Process_CBL_MEM_WRITE_CMD(void)
{
    if(BL_Host_Receive_Buffer[0] == FLASH_PAYLOAD_WRITE_FAILED)
    {
        BootloaderReply.publish("   Flash Write Failed");
    }
  return BL_Host_Receive_Buffer[0];   
}
/**********************************************************************************************************************
 *  GLOBAL FUNCTIONS
 *********************************************************************************************************************/

/* Wait for exactly one byte on Serial2, returns false on timeout */
static bool uart_read_byte(uint8_t* out, uint32_t timeout_ms) {
    unsigned long deadline = millis() + timeout_ms;
    while (!Serial2.available()) {
        if (millis() > deadline) return false;
        yield();
    }
    *out = Serial2.read();
    return true;
}

bool BL_Host_UART_Ping(void)
{
    /* ── Stage 0: Startup beacon ────────────────────────────────────────────
     * STM32 sends 0x55 immediately after UART init.
     * Filter out any glitch bytes and wait specifically for 0x55. */
    /* Stage 0: wait for startup beacon (0x55) */
    bool got_beacon = false;
    unsigned long beacon_deadline = millis() + 5000;
    while (millis() < beacon_deadline) {
        uint8_t b = 0;
        if (!uart_read_byte(&b, beacon_deadline - millis())) break;
        if (b == 0x55) { got_beacon = true; break; }
    }
    if (!got_beacon) return false;

    /* Stage 1: echo test — send 0xBE, expect 0xEF */
    bool stage1_passed = false;
    for (int attempt = 1; attempt <= 5 && !stage1_passed; attempt++) {
        delay(100);
        while (Serial2.available()) Serial2.read();
        Serial2.write(0xBE);
        Serial2.flush();
        uint8_t echo_rx = 0;
        if (uart_read_byte(&echo_rx, 2000) && echo_rx == 0xEF) {
            stage1_passed = true;
        } else if (echo_rx == CBL_SEND_NACK) {
            unsigned long dl = millis() + 200;
            while (millis() < dl) { if (Serial2.available()) Serial2.read(); else yield(); }
        }
    }
    if (!stage1_passed) return false;

    /* Stage 2: protocol ping with CRC */
    uint8_t ping_buf[6];
    ping_buf[0] = 5;
    ping_buf[1] = CBL_GET_VER_CMD;
    uint32_t crc = Calculate_CRC32(ping_buf, 2);
    ping_buf[2] = (uint8_t)(crc);
    ping_buf[3] = (uint8_t)(crc >> 8);
    ping_buf[4] = (uint8_t)(crc >> 16);
    ping_buf[5] = (uint8_t)(crc >> 24);

    while (Serial2.available()) Serial2.read();
    for (uint8_t i = 0; i < 6; i++) {
        Serial2.write(ping_buf[i]);
        Serial2.flush();
        delayMicroseconds(500);
    }

    uint8_t rx0 = 0, rx1 = 0;
    if (!uart_read_byte(&rx0, 3000)) return false;
    if (rx0 == CBL_SEND_ACK) {
        uart_read_byte(&rx1, 500);
        return true;
    }
    return false;
}

void BL_Host_Get_Version(void)
{
    uint8_t CBL_GET_VER_CMD_Len = 6;
    uint32_t CRC32_Value =0;
    uint8_t Length_To_Follow = 0;
    BL_Host_Buffer[0] = CBL_GET_VER_CMD_Len - 1;
    BL_Host_Buffer[1] = CBL_GET_VER_CMD;
    CRC32_Value = Calculate_CRC32(BL_Host_Buffer, CBL_GET_VER_CMD_Len - 4);
    BL_Host_Buffer[2] = (uint8_t)(CRC32_Value);
    BL_Host_Buffer[3] = (uint8_t)(CRC32_Value >> 8);
    BL_Host_Buffer[4] = (uint8_t)(CRC32_Value >> 16);
    BL_Host_Buffer[5] = (uint8_t)(CRC32_Value >> 24);
    Send_Data_To_Bootloader_Noprint(&BL_Host_Buffer[0], 1);
    Send_Data_To_Bootloader_Noprint(&BL_Host_Buffer[1], CBL_GET_VER_CMD_Len - 1);
    if(!Recieve_Data_From_Bootloader(2)){ BootloaderReply.publish("No response from Bootloader"); return; }
     if(BL_Host_Receive_Buffer[0] == CBL_SEND_ACK)
     {
        Serial.println("\nReceived Acknowledgement from Bootloader");
        Serial.println("\nCRC Verification Successful");
        Length_To_Follow = BL_Host_Receive_Buffer[1];
        if(!Recieve_Data_From_Bootloader(Length_To_Follow)){ BootloaderReply.publish("Bootloader data timeout"); return; }
        Process_CBL_GET_VER_CMD();
     }
     else if (BL_Host_Receive_Buffer[0] == CBL_SEND_NACK)
     {
        Serial.println("\n   Received Not-Acknowledgement from Bootloader");
        Print_NACK_Diagnostics();
        Serial.println("\n   CRC Verification Failed");
        BootloaderReply.publish("CRC Verification Failed");
     }
}
void BL_Host_Get_Chip_ID(void)
{
    uint8_t CBL_GET_CID_CMD_Len = 6;
    uint32_t CRC32_Value =0;
    uint8_t Length_To_Follow = 0;
    BL_Host_Buffer[0] = CBL_GET_CID_CMD_Len - 1;
    BL_Host_Buffer[1] = CBL_GET_CID_CMD;
    CRC32_Value = Calculate_CRC32(BL_Host_Buffer, CBL_GET_CID_CMD_Len - 4);
    BL_Host_Buffer[2] = (uint8_t)(CRC32_Value);
    BL_Host_Buffer[3] = (uint8_t)(CRC32_Value >> 8);
    BL_Host_Buffer[4] = (uint8_t)(CRC32_Value >> 16);
    BL_Host_Buffer[5] = (uint8_t)(CRC32_Value >> 24);
    Send_Data_To_Bootloader_Noprint(&BL_Host_Buffer[0], 1);
    Send_Data_To_Bootloader_Noprint(&BL_Host_Buffer[1], CBL_GET_CID_CMD_Len - 1);
    if(!Recieve_Data_From_Bootloader(2)){ BootloaderReply.publish("No response from Bootloader"); return; }
     if(BL_Host_Receive_Buffer[0] == CBL_SEND_ACK)
     {
        Serial.println("\nReceived Acknowledgement from Bootloader");
        Serial.println("\nCRC Verification Successful");
        Length_To_Follow = BL_Host_Receive_Buffer[1];
        if(!Recieve_Data_From_Bootloader(Length_To_Follow)){ BootloaderReply.publish("Bootloader data timeout"); return; }
        Process_CBL_GET_CID_CMD();
     }
     else if (BL_Host_Receive_Buffer[0] == CBL_SEND_NACK)
     {
        Serial.println("\n   Received Not-Acknowledgement from Bootloader");
        Serial.println("\n   CRC Verification Failed");
        BootloaderReply.publish("CRC Verification Failed");
     }
}
void BL_Host_Get_RDP(void)
{
    uint8_t CBL_GET_RDP_STATUS_CMD_Len = 6;
    uint32_t CRC32_Value =0;
    uint8_t Length_To_Follow = 0;
    BL_Host_Buffer[0] = CBL_GET_RDP_STATUS_CMD_Len - 1;
    BL_Host_Buffer[1] = CBL_GET_RDP_STATUS_CMD;
    CRC32_Value = Calculate_CRC32(BL_Host_Buffer, CBL_GET_RDP_STATUS_CMD_Len - 4);
    BL_Host_Buffer[2] = (uint8_t)(CRC32_Value);
    BL_Host_Buffer[3] = (uint8_t)(CRC32_Value >> 8);
    BL_Host_Buffer[4] = (uint8_t)(CRC32_Value >> 16);
    BL_Host_Buffer[5] = (uint8_t)(CRC32_Value >> 24);
    Send_Data_To_Bootloader_Noprint(&BL_Host_Buffer[0], 1);
    Send_Data_To_Bootloader_Noprint(&BL_Host_Buffer[1], CBL_GET_RDP_STATUS_CMD_Len - 1);
    if(!Recieve_Data_From_Bootloader(2)){ BootloaderReply.publish("No response from Bootloader"); return; }
     if(BL_Host_Receive_Buffer[0] == CBL_SEND_ACK)
     {
        Serial.println("\nReceived Acknowledgement from Bootloader");
        Serial.println("\nCRC Verification Successful");
        Length_To_Follow = BL_Host_Receive_Buffer[1];
        if(!Recieve_Data_From_Bootloader(Length_To_Follow)){ BootloaderReply.publish("Bootloader data timeout"); return; }
        Process_CBL_GET_RDP_CMD();
     }
     else if (BL_Host_Receive_Buffer[0] == CBL_SEND_NACK)
     {
        Serial.println("\n   Received Not-Acknowledgement from Bootloader");
        Serial.println("\n   CRC Verification Failed");
        BootloaderReply.publish("CRC Verification Failed");
     }
}

void BL_Host_Erase_Application(void)
{
   uint8_t CBL_FLASH_ERASE_CMD_Len = 6;
    uint32_t CRC32_Value =0;
    uint8_t Length_To_Follow = 0;
    BL_Host_Buffer[0] = CBL_FLASH_ERASE_CMD_Len - 1;
    BL_Host_Buffer[1] = CBL_FLASH_ERASE_CMD;
    CRC32_Value = Calculate_CRC32(BL_Host_Buffer, CBL_FLASH_ERASE_CMD_Len - 4);
    BL_Host_Buffer[2] = (uint8_t)(CRC32_Value);
    BL_Host_Buffer[3] = (uint8_t)(CRC32_Value >> 8);
    BL_Host_Buffer[4] = (uint8_t)(CRC32_Value >> 16);
    BL_Host_Buffer[5] = (uint8_t)(CRC32_Value >> 24);
    Send_Data_To_Bootloader_Noprint(&BL_Host_Buffer[0], 1);
    Send_Data_To_Bootloader_Noprint(&BL_Host_Buffer[1], CBL_FLASH_ERASE_CMD_Len - 1);
    if(!Recieve_Data_From_Bootloader(2)){ BootloaderReply.publish("No response from Bootloader"); return; }
     if(BL_Host_Receive_Buffer[0] == CBL_SEND_ACK)
     {
        Serial.println("\nReceived Acknowledgement from Bootloader");
        Serial.println("\nCRC Verification Successful");
        Length_To_Follow = BL_Host_Receive_Buffer[1];
        Serial.println("\nErasing Application .......");
        BootloaderReply.publish (" Erasing Application .......");
        Recieve_Data_From_Bootloader_Noprint(Length_To_Follow);
        Process_CBL_FLASH_ERASE_CMD();
     }
     else if (BL_Host_Receive_Buffer[0] == CBL_SEND_NACK)
     {
        Serial.println("\n   Received Not-Acknowledgement from Bootloader");
        Serial.println("\n   CRC Verification Failed");
        BootloaderReply.publish("CRC Verification Failed");
     }
}
void BL_Host_Jump_To_Application(void)
{
    uint8_t CBL_GO_TO_MAIN_APP_CMD_Len = 6;
    uint32_t CRC32_Value =0;
    uint8_t Length_To_Follow = 0;
    BL_Host_Buffer[0] = CBL_GO_TO_MAIN_APP_CMD_Len - 1;
    BL_Host_Buffer[1] = CBL_GO_TO_MAIN_APP_CMD;
    CRC32_Value = Calculate_CRC32(BL_Host_Buffer, CBL_GO_TO_MAIN_APP_CMD_Len - 4);
    BL_Host_Buffer[2] = (uint8_t)(CRC32_Value);
    BL_Host_Buffer[3] = (uint8_t)(CRC32_Value >> 8);
    BL_Host_Buffer[4] = (uint8_t)(CRC32_Value >> 16);
    BL_Host_Buffer[5] = (uint8_t)(CRC32_Value >> 24);
    Send_Data_To_Bootloader_Noprint(&BL_Host_Buffer[0], 1);
    Send_Data_To_Bootloader_Noprint(&BL_Host_Buffer[1], CBL_GO_TO_MAIN_APP_CMD_Len - 1);
    if(!Recieve_Data_From_Bootloader(2)){ BootloaderReply.publish("No response from Bootloader"); return; }
     if(BL_Host_Receive_Buffer[0] == CBL_SEND_ACK)
     {
        Serial.println("\nReceived Acknowledgement from Bootloader");
        Serial.println("\nCRC Verification Successful");
        Length_To_Follow = BL_Host_Receive_Buffer[1];
        if(!Recieve_Data_From_Bootloader(Length_To_Follow)){ BootloaderReply.publish("Bootloader data timeout"); return; }
        Process_CBL_GO_TO_MAIN_APP_CMD();
     }
     else if (BL_Host_Receive_Buffer[0] == CBL_SEND_NACK)
     {
        Serial.println("\n   Received Not-Acknowledgement from Bootloader");
        Serial.println("\n   CRC Verification Failed");
        BootloaderReply.publish("CRC Verification Failed");
     }
}
void BL_Host_Flash_Application(void)
{
    uint32_t CBL_MEM_WRITE_CMD_Len = 0;
    uint32_t CRC32_Value =0;
    uint8_t Length_To_Follow = 0;   
    uint8_t Download_status = 0;
    uint32_t File_Total_Len = 0;
    uint32_t BinFileRemainingBytes = 0;
    uint32_t BinFileSentBytes = 0;
    uint32_t BaseMemoryAddress = 0;
    uint32_t BinFileReadLength = 0;
    uint8_t BinFileByteValue = 0;
    uint8_t Flash_write_status = 0;
    uint32_t Uploading_percentage =0;
    uint32_t Uploading_percentage_temp =0;
    char uploading_percentage_str[55] = {0};
     BootloaderReply.publish("Downloading Application .....");
    Download_status = FireBase_DownloadFile("FOTA_Application.bin", "/update.bin");
    if (Download_status == DOWNLOAD_FAILED)
    {
        BootloaderReply.publish("Application Download Failed");
    }
    else
    {
        BootloaderReply.publish("Application Download Successful");
    }
    File file = LittleFS.open("/update.bin", "r");
   // Get the total length of the binary file 
    File_Total_Len =  file.size();
    Serial.printf("Preparing writing a binary file with length %d Bytes\n\n",File_Total_Len);
    // Calculate the remaining payload 
    BinFileRemainingBytes = File_Total_Len;
    // Get the start address to write the payload 
    BaseMemoryAddress = 0x08004000;
    // Keep sending the write packet till the last payload byte
    delay(2000);
    while(BinFileRemainingBytes)
    {
        if(BinFileRemainingBytes >= 128)
        {
            BinFileReadLength = 128;
        }
        else
        {
            BinFileReadLength = BinFileRemainingBytes;
        }
        for (uint8_t BinFileByte = 0; BinFileByte < BinFileReadLength; BinFileByte++)
        {
            BinFileByteValue = file.read();
            BL_Host_Buffer[7 + BinFileByte] = BinFileByteValue;
        }
        BL_Host_Buffer[1] = CBL_MEM_WRITE_CMD;
        //Update the Host packet with the base address
        BL_Host_Buffer[2] = (uint8_t)(BaseMemoryAddress);
        BL_Host_Buffer[3] = (uint8_t)(BaseMemoryAddress >> 8);
        BL_Host_Buffer[4] = (uint8_t)(BaseMemoryAddress >> 16);
        BL_Host_Buffer[5] = (uint8_t)(BaseMemoryAddress >> 24);
     // Update the Host packet with the payload length 
        BL_Host_Buffer[6] = BinFileReadLength;
    // Update the Host packet with the packet length
        CBL_MEM_WRITE_CMD_Len = (BinFileReadLength + 11);
        BL_Host_Buffer[0] = CBL_MEM_WRITE_CMD_Len - 1;
        CRC32_Value = Calculate_CRC32(BL_Host_Buffer, CBL_MEM_WRITE_CMD_Len - 4);
        BL_Host_Buffer[7 + BinFileReadLength] = (uint8_t)(CRC32_Value);
        BL_Host_Buffer[8 + BinFileReadLength] = (uint8_t)(CRC32_Value >> 8);
        BL_Host_Buffer[9 + BinFileReadLength] = (uint8_t)(CRC32_Value >> 16);
        BL_Host_Buffer[10 + BinFileReadLength] = (uint8_t)(CRC32_Value >> 24);
          Uploading_percentage = (100 * (BinFileSentBytes) / File_Total_Len);
        if (Uploading_percentage != Uploading_percentage_temp)
        {
            sprintf(uploading_percentage_str, "Uploading Application .... ( %d %% )", Uploading_percentage);
            if(Uploading_percentage%10 == 0)
            {
            BootloaderReply.publish(uploading_percentage_str);
            }
            else if(Uploading_percentage ==99)
            {
            BootloaderReply.publish("Uploading Application .... ( 100 % )");
            }
            Serial.print(uploading_percentage_str);
            Serial.println();
        }
        Uploading_percentage_temp = Uploading_percentage;
     // Send the packet length to the bootloader  
        Send_Data_To_Bootloader_Noprint(&BL_Host_Buffer[0], 1);
        Send_Data_To_Bootloader_Noprint(&BL_Host_Buffer[1], CBL_MEM_WRITE_CMD_Len - 1);
     // Calculate the next Base memory address '''
        BaseMemoryAddress = BaseMemoryAddress + BinFileReadLength;    
    // Update the total number of bytes sent to the bootloader
        BinFileSentBytes = BinFileSentBytes + BinFileReadLength;
    //Calculate the remaining payload 
        BinFileRemainingBytes = File_Total_Len - BinFileSentBytes;
        if(!Recieve_Data_From_Bootloader_Noprint(2)){
            BootloaderReply.publish("Bootloader write timeout");
            break;
        }
     if(BL_Host_Receive_Buffer[0] == CBL_SEND_ACK)
     {
        Length_To_Follow = BL_Host_Receive_Buffer[1];
        Recieve_Data_From_Bootloader_Noprint(Length_To_Follow);
        Flash_write_status = Process_CBL_MEM_WRITE_CMD();
        if (Flash_write_status == FLASH_PAYLOAD_WRITE_FAILED)
        {
           break;
        }
     }
     else if (BL_Host_Receive_Buffer[0] == CBL_SEND_NACK)
     {
        Serial.println("\n   Received Not-Acknowledgement from Bootloader");
        Serial.println("\n   CRC Verification Failed");
        BootloaderReply.publish("CRC Verification Failed");
        break;
     }
    }
    if (Flash_write_status == FLASH_PAYLOAD_WRITE_PASSED)
    {
        Serial.println("\nApplication Uploaded Successfully");
        BootloaderReply.publish("Application Uploaded Successfully");
    }
}
/**********************************************************************************************************************
 *  END OF FILE: Bootloader_Host.cpp
 *********************************************************************************************************************/
