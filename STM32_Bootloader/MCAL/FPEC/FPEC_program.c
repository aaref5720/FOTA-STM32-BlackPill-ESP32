/*******************************************************************************
 * FILE DESCRIPTION
 *******************************************************************************
 *  File:   FPEC_program.c
 *  Author: Abdelrahman Mohamed
 *  Date:   May 8, 2026
 *  Description: <Write File DESCRIPTION here>
 ******************************************************************************/

/*******************************************************************************
 *  INCLUDES
 ******************************************************************************/
#include "FPEC_interface.h"
#include "FPEC_private.h"
/*******************************************************************************
 *  LOCAL MACROS CONSTANT\FUNCTION
 ******************************************************************************/

/*******************************************************************************
 *  LOCAL DATA
 ******************************************************************************/

/*******************************************************************************
 *  GLOBAL DATA
 ******************************************************************************/

/*******************************************************************************
 *  LOCAL FUNCTION PROTOTYPES
 ******************************************************************************/
static void FLASH_UNLOCK(void);
static void FLASH_LOCK(void);
static void WAIT_FOR_BSY(void);

/*******************************************************************************
 *  LOCAL FUNCTION
 ******************************************************************************/
static void FLASH_UNLOCK(void){
	FPEC->KEYR = FLASH_KEY1;
	FPEC->KEYR = FLASH_KEY2;
}

static void FLASH_LOCK(void){
	SET_BIT(FPEC->CR, FLASH_LOCK_KEY_POS);
}
static void WAIT_FOR_BSY(void){
	while(GET_BIT(FPEC->SR, FLASH_BSY_FLAG_BIT_POS) == 1);
}

/*******************************************************************************
 *  GLOBAL FUNCTION
 ******************************************************************************/
void FPEC_Write(u32 copy_u32Adress, u8* copy_u32Data, u32 copy_u32DataLength){
    u32 Local_Counter = 0;
    FLASH_UNLOCK();

    for(Local_Counter = 0; Local_Counter < copy_u32DataLength; Local_Counter += 4){
        WAIT_FOR_BSY();
        // Clear any previous errors
        if(GET_BIT(FPEC->SR, FLASH_PGERR_BIT_POS)) SET_BIT(FPEC->SR, FLASH_PGERR_BIT_POS);
        if(GET_BIT(FPEC->SR, FLASH_WRPERR_BIT_POS)) SET_BIT(FPEC->SR, FLASH_WRPERR_BIT_POS);
        MODIFY_REG(FPEC->CR, 0x3 << 8, 0x2 << 8); // PSIZE = word (32-bit)
        SET_BIT(FPEC->CR, FLASH_PROGRAMMING_ACTIVE_BIT_POS);
        *((volatile u32*)copy_u32Adress) = *(u32*)&copy_u32Data[Local_Counter];
        WAIT_FOR_BSY();
        CLR_BIT(FPEC->CR, FLASH_PROGRAMMING_ACTIVE_BIT_POS);
        copy_u32Adress += 4;
    }
    FLASH_LOCK();
}

void FPEC_EraseSectors(u8 copy_u8SectorNumber, u8 copy_u8NumofSectors){
	u32 Local_Counter = 0;
	FLASH_UNLOCK();

	for(Local_Counter = 0; Local_Counter < copy_u8NumofSectors; Local_Counter++){
		WAIT_FOR_BSY();
		SET_BIT(FPEC->CR, SECTOR_ERASE_ACTIVE_BIT_POS);
		MODIFY_REG(FPEC->CR, 0xF<<3, copy_u8SectorNumber<<3);
		SET_BIT(FPEC->CR, FLASH_ERASE_START_BIT_POS);
		WAIT_FOR_BSY();
		CLR_BIT(FPEC->CR, SECTOR_ERASE_ACTIVE_BIT_POS);
		copy_u8SectorNumber++;
	}
	FLASH_LOCK();
}

u8 FPEC_Get_ReadProtectionLevel(void){
	return ((FPEC->OPTCR >> 8) & 0xFF);
}
/*******************************************************************************
 *  END OF FILE: FPEC_program.c
 ******************************************************************************/
