/*******************************************************************************
 * FILE DESCRIPTION
 *******************************************************************************
 *  File:        FPEC_private.h
 *  Author:      Abdelrahman Mohamed
 *  Date:        May 8, 2026
 *  Description: <Write File DESCRIPTION here>
 ******************************************************************************/

#ifndef MCAL_FPEC_FPEC_PRIVATE_H_
#define MCAL_FPEC_FPEC_PRIVATE_H_

/******************************************************************************
 *  INCLUDES
 ******************************************************************************/
#include "LIB/BIT_MATH.h"
#include "LIB/STD_TYPES.h"

/******************************************************************************
 *  GLOBAL CONSTANT MACROS
 ******************************************************************************/
#define FPEC_BASE	0x40023c00
#define FPEC ((FPEC_t*)FPEC_BASE)

#define FLASH_KEY1  					 0x45670123
#define FLASH_KEY2  					 0xCDEF89AB
#define FLASH_LOCK_KEY_POS 				 31

#define FLASH_PROGRAMMING_ACTIVE_BIT_POS 0
#define FLASH_BSY_FLAG_BIT_POS 			 16

#define SECTOR_ERASE_ACTIVE_BIT_POS 	 1
#define FLASH_ERASE_START_BIT_POS 	 	 16

#define FLASH_PGERR_BIT_POS 5
#define FLASH_WRPERR_BIT_POS 4
/******************************************************************************
 *  GLOBAL FUNCTION MACROS
 ******************************************************************************/

/******************************************************************************
 *  GLOBAL DATA TYPES AND STRUCTURES
 ******************************************************************************/
typedef struct {
	volatile u32 ACR;
	volatile u32 KEYR;
	volatile u32 OPTKEYR;
	volatile u32 SR;
	volatile u32 CR;
	volatile u32 OPTCR;
}FPEC_t;
/******************************************************************************
 *  GLOBAL DATA PROTOTYPES
 ******************************************************************************/

/******************************************************************************
 *  GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************/

#endif /* MCAL_FPEC_FPEC_PRIVATE_H_ */

/******************************************************************************
 *  END OF FILE: FPEC_private.h
 ******************************************************************************/