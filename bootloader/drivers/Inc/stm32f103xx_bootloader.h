/*
 * stm32f103xx_bootloader.h
 *
 *  Created on: Sep 11, 2025
 *      Author: nphuc
 */

#ifndef INC_STM32F103XX_BOOTLOADER_H_
#define INC_STM32F103XX_BOOTLOADER_H_

#include "stm32f103xx.h"

void Bootloader_Reset();

void Bootloader_JumpApp(uint32_t appAddress);

uint8_t Bootloader_CheckApp(uint32_t appAddress, uint32_t appAddressEnd);

#endif /* INC_STM32F103XX_BOOTLOADER_H_ */
