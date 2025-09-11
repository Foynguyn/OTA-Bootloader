/*
 * stm32f103xx_flash_driver.c
 *
 *  Created on: Jul 31, 2025
 *      Author: nphuc
 */

#include "stm32f103xx_flash_driver.h"

void FLASH_Unlock(){
    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;
}

void FLASH_Lock(){
    FLASH->CR |= (1 << FLASH_CR_LOCK);
}

uint8_t FLASH_WriteData(uint32_t PageAddress, uint32_t *pBuffer, uint16_t length){
	FLASH_Unlock();
    while(length > 0){
    	uint8_t currentPage = (PageAddress / 0x0400) & 0x0FF;
    	if(currentPage > 127) return FLASH_ERROR;

    	uint32_t value = (*pBuffer);
        uint16_t lower_half = (uint16_t)(value & 0xFFFF);
        uint16_t upper_half = (uint16_t)((value >> 16) & 0xFFFF);

        /* Clear flags */
        FLASH->SR |= (1 << FLASH_SR_EOP) | (1 << FLASH_SR_PGERR) | (1 << FLASH_SR_WRPRTERR);

        FLASH->CR |= (1 << FLASH_CR_PG); /* flash programming mode */
        /* Write lower_half */
        *(uint16_t *)PageAddress = lower_half;
        while (FLASH->SR & FLASH_SR_BSY);

        /* Write upper_half */
        *(uint16_t *)(PageAddress + 2) = upper_half;
        while (FLASH->SR & FLASH_SR_BSY);
        FLASH->CR &= ~(1 << FLASH_CR_PG);

        if(((FLASH->SR >> FLASH_SR_PGERR) & 1) || (FLASH->SR >> FLASH_SR_WRPRTERR) & 1)
            return FLASH_ERROR;

        /* step to next address */
        pBuffer++;
        PageAddress+= 4;
        length-= 1;
    }

    FLASH_Lock();
    return FLASH_OK;
}

void FLASH_ReadData(uint32_t PageAddress, uint32_t *pBuffer, uint16_t length){
    while(length > 0)
    {
        /* Read lower_half */
        uint16_t lower_half = *(volatile uint16_t *)PageAddress;

        /* Read upper_half */
        uint16_t upper_half = *(volatile uint16_t *)(PageAddress + 2);

        (*pBuffer) = (upper_half << 16) | lower_half;

        pBuffer++;
        PageAddress += 4;
        length--;
    }
}

void FLASH_Erase(uint32_t PageAdress) {
	/* choose page erase mode */
	FLASH->CR |= (1 << FLASH_CR_PER);
	/* select page address to erase */
	FLASH->AR = PageAdress;

	FLASH->CR |= (1 << FLASH_CR_STRT);

	/* wait BSY reset */
	while((FLASH->SR >> FLASH_SR_BSY) & 1);

	FLASH->CR &= ~(1 << FLASH_CR_PER);
}

void FLASH_RemovePartition(uint32_t address, uint8_t numOfPage){
	FLASH_Unlock();
	for(uint8_t i = 0; i < numOfPage; i++){
		FLASH_Erase(address);
		address+= 0x400;
	}
	FLASH_Lock();
}
