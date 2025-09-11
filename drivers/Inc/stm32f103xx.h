/*
 * stm32f103xx.h
 *
 *  Created on: Jun 10, 2025
 *      Author: nphuc
 */

#ifndef INC_STM32F103XX_H_
#define INC_STM32F103XX_H_

#define __vo				volatile
#define __weak				__attribute__((weak))

/*
 * base addresses of Flash and SRAM memories
 */
#include <stddef.h>
#include <stdint.h>

#define FLASH_BASEADDR		0x08000000U
#define SCB_BASEADDR		0xE000ED00U

/*
 * AHBx and APBx Bus Peripheral base addresses
 */

#define PERIPH_BASE			0x40000000U
#define APB1PERIPH_BASE		PERIPH_BASE
#define APB2PERIPH_BASE		0x40010000U
#define AHBPERIPH_BASE		0x40018000U

/*
 * Base addresses of peripherals which are hanging on AHB bus
 */

#define RCC_BASEADDR		(AHBPERIPH_BASE + 0x9000)

/* Base Address of peripheral which are hanging on APB2 */
#define GPIOA_BASEADDR					(APB2PERIPH_BASE + 0x0800)
#define GPIOB_BASEADDR					(APB2PERIPH_BASE + 0x0C00)
#define GPIOC_BASEADDR					(APB2PERIPH_BASE + 0x1000)

#define USART1_BASEADDR					(APB2PERIPH_BASE + 0x3800)

#define NVIC_BASE_ADDR					0xE000E100U

#define ENABLE							1
#define DISABLE							0
#define SET 							ENABLE
#define RESET 							DISABLE

#include "stm32f103xx_core_driver.h"
#include "stm32f103xx_rcc_driver.h"
#include "stm32f103xx_gpio_driver.h"
#include "stm32f103xx_usart_driver.h"
#include "stm32f103xx_flash_driver.h"
#include "stm32f103xx_bootloader.h"

#endif /* INC_STM32F103XX_H_ */
