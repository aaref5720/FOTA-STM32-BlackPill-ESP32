/*******************************************************************************
 * FILE DESCRIPTION
 *******************************************************************************
 *  File:   Bootloader.c (Modified)
 *  Author: Abdelrahman Mohamed
 *  Date:   May 8, 2026
 *  Description: Modified Bootloader with security fixes
 ******************************************************************************/

/*******************************************************************************
 *  INCLUDES
 ******************************************************************************/
#include "Bootloader.h"

/*******************************************************************************
 *  LOCAL MACROS CONSTANT\FUNCTION
 ******************************************************************************/

/*******************************************************************************
 *  LOCAL DATA
 ******************************************************************************/
static u8 BL_HOST_RX_BUFFER[200];
static u32 BL_Debug_Host_CRC = 0;
static u32 BL_Debug_MCU_CRC = 0;
static u8 BL_Debug_CRC_Size = 0;
/*******************************************************************************
 *  GLOBAL DATA
 ******************************************************************************/

/*******************************************************************************
 *  LOCAL FUNCTION PROTOTYPES
 ******************************************************************************/
static void Bootloader_Get_Version(u8* BL_HOST_BUFFER);
static void Bootloader_Get_CHIP_ID(u8* BL_HOST_BUFFER);
static void Bootloader_Get_RDP_Status(u8* BL_HOST_BUFFER);
static void Bootloader_Application_Erase(u8* BL_HOST_BUFFER);
static void Bootloader_Upload_Application(u8* BL_HOST_BUFFER);
static void Bootloader_Jump_To_Application(u8* BL_HOST_BUFFER);


static u8 Bootloader_CRC_Verify(u8* CRC_Data, u32 CRC_Data_size, u32 CRC_Verfied);
static void Bootloader_Send_ACK(u8 Datalength);
static void Bootloader_Send_NACK(void);
static void Bootloader_Send_Data_TO_Host(u8* Host_Buffer, u32 Host_Buffer_Length);
static u8 Host_Adress_Verification(u32 Host_Adress);
/*******************************************************************************
 *  LOCAL FUNCTION
 ******************************************************************************/
static u8 Host_Adress_Verification(u32 Host_Adress){
	u8 Adress_Verification = ADDRESS_VERIFICATION_INVALID;
	// Fixed: Start from APPLICATION_START_ADDRESS to protect bootloader sectors
	if(Host_Adress >= APPLICATION_START_ADDRESS && Host_Adress <= STM32F401CCx_FLASH_END)
	{
		Adress_Verification = ADDRESS_VERIFICATION_VALID;
	}
	return Adress_Verification;
}
static void Bootloader_Send_Data_TO_Host(u8* Host_Buffer, u32 Host_Buffer_Length){
	UART_voidTransmit(BL_HOST_COMMUNICATION_UART, Host_Buffer, Host_Buffer_Length, UART_MAX_DELAY);
}
static u8 Bootloader_CRC_Verify(u8* CRC_Data, u32 CRC_Data_Size, u32 Host_CRC)
{
    u8 CRC_Status = CRC_VERIFICATION_FAILED;
    u32 MCU_CRC_Calculated = 0;
    u8 Data_Counter = 0;
    u32 Data_Buffer = 0;

    CRC_RESET_DR();
    for(Data_Counter = 0; Data_Counter < CRC_Data_Size; Data_Counter++)
    {
        Data_Buffer = CRC_Data[Data_Counter];
        MCU_CRC_Calculated = CRC_Accumlate(&Data_Buffer, 1);
    }

    CRC_RESET_DR();

    if(Host_CRC == MCU_CRC_Calculated){
        CRC_Status = CRC_VERIFICATION_PASSED;
    }
    else{
        CRC_Status = CRC_VERIFICATION_FAILED;
    }

    return CRC_Status;
}

static void Bootloader_Send_ACK(u8 Datalength){
	u8 ACK_Arr[2] = {0};
	ACK_Arr[0] = CBL_SEND_ACK;
	ACK_Arr[1] = Datalength;
	Bootloader_Send_Data_TO_Host(ACK_Arr, 2);
}
static void Bootloader_Send_NACK(void){
	u8 NACK_Data[19] = {0};
	NACK_Data[0] = CBL_SEND_NACK;
	NACK_Data[1] = BL_HOST_RX_BUFFER[0];
	NACK_Data[2] = BL_HOST_RX_BUFFER[1];
	NACK_Data[3] = BL_Debug_CRC_Size;
	NACK_Data[4] = (u8)(BL_Debug_Host_CRC);
	NACK_Data[5] = (u8)(BL_Debug_Host_CRC >> 8);
	NACK_Data[6] = (u8)(BL_Debug_Host_CRC >> 16);
	NACK_Data[7] = (u8)(BL_Debug_Host_CRC >> 24);
	NACK_Data[8] = (u8)(BL_Debug_MCU_CRC);
	NACK_Data[9] = (u8)(BL_Debug_MCU_CRC >> 8);
	NACK_Data[10] = (u8)(BL_Debug_MCU_CRC >> 16);
	NACK_Data[11] = (u8)(BL_Debug_MCU_CRC >> 24);
	NACK_Data[12] = BL_HOST_RX_BUFFER[0];
	NACK_Data[13] = BL_HOST_RX_BUFFER[1];
	NACK_Data[14] = BL_HOST_RX_BUFFER[2];
	NACK_Data[15] = BL_HOST_RX_BUFFER[3];
	NACK_Data[16] = BL_HOST_RX_BUFFER[4];
	NACK_Data[17] = BL_HOST_RX_BUFFER[5];
	NACK_Data[18] = 0xEE;
	Bootloader_Send_Data_TO_Host(NACK_Data, sizeof(NACK_Data));
}

static void Bootloader_Get_Version(u8* BL_HOST_BUFFER){
	u8 BL_Version[4] = {CBL_VENDOR_ID, CBL_SW_MAJOR_VERSION, CBL_SW_MINOR_VERSION, CBL_SW_PATCH_VERSION};
	u16 Host_CMD_Packet_Len  = BL_HOST_BUFFER[0] + 1;
	u32 Host_CRC32 = 0;
	Host_CRC32 = *((u32*)(BL_HOST_BUFFER + Host_CMD_Packet_Len - 4));

	/****************CRC Verification***********************/
	if(Bootloader_CRC_Verify(BL_HOST_BUFFER, Host_CMD_Packet_Len - 4, Host_CRC32)
	   == CRC_VERIFICATION_PASSED){
		Bootloader_Send_ACK(4);
		Bootloader_Send_Data_TO_Host(BL_Version, 4);
	}
	else{
		Bootloader_Send_NACK();
	}
}

static void Bootloader_Get_CHIP_ID(u8* BL_HOST_BUFFER){
	u16 MCU_Identification_Number = DBGMCU_IDCODE & 0x00000FFF;
	u16 Host_CMD_Packet_Len  = BL_HOST_BUFFER[0] + 1;
	u32 Host_CRC32 = 0;
	Host_CRC32 = *((u32*)(BL_HOST_BUFFER + Host_CMD_Packet_Len - 4));

	/****************CRC Verification***********************/
	if(Bootloader_CRC_Verify(BL_HOST_BUFFER, Host_CMD_Packet_Len - 4, Host_CRC32)
	   == CRC_VERIFICATION_PASSED){
		Bootloader_Send_ACK(2);
		Bootloader_Send_Data_TO_Host((u8*)&MCU_Identification_Number, 2);
	}
	else{
		Bootloader_Send_NACK();
	}
}
static void Bootloader_Get_RDP_Status(u8* BL_HOST_BUFFER){
	u8 RDP_Level = FPEC_Get_ReadProtectionLevel();
	u16 Host_CMD_Packet_Len  = BL_HOST_BUFFER[0] + 1;
	u32 Host_CRC32 = 0;
	Host_CRC32 = *((u32*)(BL_HOST_BUFFER + Host_CMD_Packet_Len - 4));

	/****************CRC Verification***********************/
	if(Bootloader_CRC_Verify(BL_HOST_BUFFER, Host_CMD_Packet_Len - 4, Host_CRC32)
	   == CRC_VERIFICATION_PASSED){
		Bootloader_Send_ACK(1);
		Bootloader_Send_Data_TO_Host(&RDP_Level, 1);
	}
	else{
		Bootloader_Send_NACK();
	}
}
static void Bootloader_Application_Erase(u8* BL_HOST_BUFFER){
	u8 Erase_Status = SUCCESSFULL_ERASE;
	u16 Host_CMD_Packet_Len  = BL_HOST_BUFFER[0] + 1;
	u32 Host_CRC32 = 0;
	Host_CRC32 = *((u32*)(BL_HOST_BUFFER + Host_CMD_Packet_Len - 4));

	/****************CRC Verification***********************/
	if(Bootloader_CRC_Verify(BL_HOST_BUFFER, Host_CMD_Packet_Len - 4, Host_CRC32)
	   == CRC_VERIFICATION_PASSED){
		Bootloader_Send_ACK(1);
		FPEC_EraseSectors(APPLICATION_SECTOR_NUMBER, FLASH_NB_OF_SECTORS - APPLICATION_SECTOR_NUMBER);
		Bootloader_Send_Data_TO_Host(&Erase_Status, 1);
	}
	else{
		Bootloader_Send_NACK();
	}
}
static void Bootloader_Upload_Application(u8* BL_HOST_BUFFER){
	u32 HOST_Adress = *((u32*)&BL_HOST_BUFFER[2]);
	u8 Payload_Len = BL_HOST_BUFFER[6];
	u8 Address_verfication = Host_Adress_Verification(HOST_Adress);
	u8 Write_Status = FLASH_PAYLOAD_WRITE_FAILED;

	u16 Host_CMD_Packet_Len  = BL_HOST_BUFFER[0] + 1;
	u32 Host_CRC32 = 0;
	Host_CRC32 = *((u32*)(BL_HOST_BUFFER + Host_CMD_Packet_Len - 4));

	/****************CRC Verification***********************/
	if(Bootloader_CRC_Verify(BL_HOST_BUFFER, Host_CMD_Packet_Len - 4, Host_CRC32)
	   == CRC_VERIFICATION_PASSED){
		Bootloader_Send_ACK(1);
		if(Address_verfication == ADDRESS_VERIFICATION_VALID){
			FPEC_Write(HOST_Adress, &BL_HOST_BUFFER[7], Payload_Len);
			Write_Status = FLASH_PAYLOAD_WRITE_PASSED;
			Bootloader_Send_Data_TO_Host(&Write_Status, 1);
		}
		else {
			Bootloader_Send_Data_TO_Host(&Write_Status, 1);
		}

	}
	else{
		Bootloader_Send_NACK();
	}
}
static void Bootloader_Jump_To_Application(u8* BL_HOST_BUFFER){
	pMainApp ResetHandler_Address = NULL;
	u8 Jump_Status = JUPM_FAILED;
	u32 temp = 0;

	u16 Host_CMD_Packet_Len  = BL_HOST_BUFFER[0] + 1;
	u32 Host_CRC32 = 0;
	Host_CRC32 = *((u32*)(BL_HOST_BUFFER + Host_CMD_Packet_Len - 4));

	/****************CRC Verification***********************/
	if(Bootloader_CRC_Verify(BL_HOST_BUFFER, Host_CMD_Packet_Len - 4, Host_CRC32)
	   == CRC_VERIFICATION_PASSED){
		Bootloader_Send_ACK(1);
		temp = *((volatile u32*)(APPLICATION_START_ADDRESS + 4));
		ResetHandler_Address = (pMainApp) temp;
		if(temp == 0xFFFFFFFF){
			Bootloader_Send_Data_TO_Host(&Jump_Status, 1);
		}
		else{
			Jump_Status = JUPM_SUCCESS;
			Bootloader_Send_Data_TO_Host(&Jump_Status, 1);
			SCB->VTOR = APPLICATION_START_ADDRESS;
			ResetHandler_Address();
		}
	}
	else{
		Bootloader_Send_NACK();
	}
}
/*******************************************************************************
 *  GLOBAL FUNCTION
 ******************************************************************************/
void BL_UART_Fetch_Host_Command(void){
	Std_ReturnType Local_u8Error = STD_OK;
	memset(BL_HOST_RX_BUFFER, 0, sizeof(BL_HOST_RX_BUFFER));

	Local_u8Error = UART_ReceiveByte(BL_HOST_COMMUNICATION_UART, &BL_HOST_RX_BUFFER[0], UART_MAX_DELAY);
	if(Local_u8Error != STD_OK){
		return;
	}

	/* Echo test: send 0xBE -> receive 0xEF, confirms UART works without CRC */
	if(BL_HOST_RX_BUFFER[0] == 0xBE){
		u8 echo = 0xEF;
		UART_voidTransmit(BL_HOST_COMMUNICATION_UART, &echo, 1, UART_MAX_DELAY);
		return;
	}

	if(BL_HOST_RX_BUFFER[0] == 0 || BL_HOST_RX_BUFFER[0] >= BL_HOST_BUFFER_RX_LENGTH){
		return;
	}

	/* Finite timeout so STM32 can recover if a partial/invalid frame arrives */
	Local_u8Error = UART_voidReceive(BL_HOST_COMMUNICATION_UART, &BL_HOST_RX_BUFFER[1], BL_HOST_RX_BUFFER[0], 1000000UL);
	if(Local_u8Error != STD_OK){
		return;
	}

	switch(BL_HOST_RX_BUFFER[1]){
	case CBL_GET_VER_CMD:
		Bootloader_Get_Version(BL_HOST_RX_BUFFER);
		break;
	case CBL_GET_CID_CMD:
		Bootloader_Get_CHIP_ID(BL_HOST_RX_BUFFER);
		break;
	case CBL_GET_RDP_STATUS_CMD:
		Bootloader_Get_RDP_Status(BL_HOST_RX_BUFFER);
		break;
	case CBL_FLASH_ERASE_CMD:
		Bootloader_Application_Erase(BL_HOST_RX_BUFFER);
		break;
	case CBL_MEM_WRITE_CMD:
		Bootloader_Upload_Application(BL_HOST_RX_BUFFER);
		break;
	case CBL_GO_TO_MAIN_APP_CMD:
		Bootloader_Jump_To_Application(BL_HOST_RX_BUFFER);
		break;
	default:
		Bootloader_Send_NACK();
		break;
	}
}
