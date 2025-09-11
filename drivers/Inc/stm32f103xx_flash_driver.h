/*
 * stm32f103xx_flash_driver.h
 *
 *  Created on: Jul 31, 2025
 *      Author: nphuc
 */

#ifndef INC_STM32F103XX_FLASH_DRIVER_H_
#define INC_STM32F103XX_FLASH_DRIVER_H_

#include "stm32f103xx.h"

#define FLASH						((FLASH_TypeDef_t*)(FLASH_BASEADDR + 0x38022000U))

#define FLASH_SR_BSY				0
#define FLASH_SR_PGERR				1
#define FLASH_SR_WRPRTERR			2
#define FLASH_SR_EOP				3

#define FLASH_CR_PG					0
#define FLASH_CR_PER				1
#define FLASH_CR_MER				2
#define FLASH_CR_OPTPG				4
#define FLASH_CR_OPTER				5
#define FLASH_CR_STRT				6
#define FLASH_CR_LOCK				7
#define FLASH_CR_OPTWRE				9
#define FLASH_CR_ERRIE				10
#define FLASH_CR_EOPIE				12

#define FLASH_OK					1
#define FLASH_ERROR					0

/* peripheral register definition structure for FLASH */

typedef struct{
	__vo uint32_t ACR;
	__vo uint32_t KEYR;
	__vo uint32_t OPTKEYR;
	__vo uint32_t SR;
	__vo uint32_t CR;
	__vo uint32_t AR;
	__vo uint32_t RESERVED;
	__vo uint32_t OBR;
	__vo uint32_t WRPR;
} FLASH_TypeDef_t;

void FLASH_Unlock();

void FLASH_Lock();

uint8_t FLASH_WriteData(uint32_t PageAddress, uint32_t *pBuffer, uint16_t length);

void FLASH_ReadData(uint32_t PageAddress, uint32_t *pBuffer, uint16_t length);

void FLASH_Erase(uint32_t PageAdress);

void FLASH_RemovePartition(uint32_t address, uint8_t numOfPage);

#endif /* INC_STM32F103XX_FLASH_DRIVER_H_ */
