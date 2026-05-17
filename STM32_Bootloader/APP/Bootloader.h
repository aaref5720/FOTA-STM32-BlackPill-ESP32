/*******************************************************************************
 * FILE DESCRIPTION
 *******************************************************************************
 *  File:        Bootloader.h
 *  Author:      Abdelrahman Mohamed
 *  Date:        May 8, 2026
 *  Description: <Write File DESCRIPTION here>
 ******************************************************************************/

#ifndef APP_BOOTLOADER_H_
#define APP_BOOTLOADER_H_

/******************************************************************************
 *  INCLUDES
 ******************************************************************************/
#include <string.h>
#include "MCAL/UART/UART_Lcfg.h"
#include "MCAL/UART/UART_interface.h"
#include "MCAL/RCC/RCC_interface.h"
#include "MCAL/SCB/SCB_private.h"
#include "MCAL/FPEC/FPEC_interface.h"
#include "MCAL/CRC/CRC_interface.h"

/******************************************************************************
 *  GLOBAL CONSTANT MACROS
 ******************************************************************************/
#define BL_HOST_BUFFER_RX_LENGTH 200
#define BL_HOST_COMMUNICATION_UART &UART1_Cfg
#define DBGMCU_IDCODE_BASE		0xE0042000
#define DBGMCU_IDCODE (*(volatile u32*)DBGMCU_IDCODE_BASE)

#define CBL_GET_VER_CMD			0x10
#define CBL_GET_CID_CMD			0x12
#define CBL_GET_RDP_STATUS_CMD	0x13
#define CBL_FLASH_ERASE_CMD		0x15
#define CBL_MEM_WRITE_CMD		0x16
#define CBL_GO_TO_MAIN_APP_CMD	0x18

#define CBL_VENDOR_ID			200
#define CBL_SW_MAJOR_VERSION 	1
#define CBL_SW_MINOR_VERSION	0
#define CBL_SW_PATCH_VERSION	0


#define CRC_VERIFICATION_FAILED 0
#define CRC_VERIFICATION_PASSED 1

#define CBL_SEND_NACK			0xAB
#define CBL_SEND_ACK			0xCD

#define APPLICATION_SECTOR_NUMBER 1
#define FLASH_NB_OF_SECTORS		  6

#define ADDRESS_VERIFICATION_VALID		0x01
#define ADDRESS_VERIFICATION_INVALID	0x00
#define SUCCESSFULL_ERASE		0x03

#define FLASH_PAYLOAD_WRITE_FAILED   0x00
#define FLASH_PAYLOAD_WRITE_PASSED   0x01

#define FLASH_PAYLOAD_INVALID_LENGTH 0x02
#define FLASH_PAYLOAD_INVALID_ADDRESS 0x03

#define FLASH_BASE              0x08000000U
#define STM32F401CCx_FLASH_SIZE (256 * 1024)
#define STM32F401CCx_FLASH_END  (FLASH_BASE + STM32F401CCx_FLASH_SIZE)

#define JUPM_FAILED		0x00
#define JUPM_SUCCESS 	0x01

#define APPLICATION_START_ADDRESS	0x08004000
/******************************************************************************
 *  GLOBAL FUNCTION MACROS
 ******************************************************************************/

/******************************************************************************
 *  GLOBAL DATA TYPES AND STRUCTURES
 ******************************************************************************/
typedef void (*pMainApp)(void);
/******************************************************************************
 *  GLOBAL DATA PROTOTYPES
 ******************************************************************************/

/******************************************************************************
 *  GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************/
void BL_UART_Fetch_Host_Command(void) ;

#endif /* APP_BOOTLOADER_H_ */

/******************************************************************************
 *  END OF FILE: Bootloader.h
 ******************************************************************************/
