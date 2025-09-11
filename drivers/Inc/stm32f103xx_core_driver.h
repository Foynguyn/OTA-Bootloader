/*
 * stm32f103xx_interrupt_driver.h
 *
 *  Created on: Aug 5, 2025
 *      Author: nphuc
 */

#ifndef INC_STM32F103XX_CORE_DRIVER_H_
#define INC_STM32F103XX_CORE_DRIVER_H_

#include "stm32f103xx.h"

#define IRQ_NO_USART1		37

#define AIRCR_VECTKEY     	16
#define AIRCR_SYSRESETREQ	2

/* peripheral register definition structure for NVIC */
typedef struct{
	__vo uint32_t ISER[8];
	__vo uint32_t RESERVED0[24];
	__vo uint32_t ICER[8];
	__vo uint32_t RESERVED1[24];
	__vo uint32_t ISPR[8];
	__vo uint32_t RESERVED2[24];
	__vo uint32_t ICPR[8];
	__vo uint32_t RESERVED3[24];
	__vo uint32_t IABR[8];
	__vo uint32_t RESERVED4[56];
	__vo uint32_t IPR[60];
	__vo uint32_t RESERVED5[644];
	__vo uint32_t STIR;
} NVIC_TypeDef_t;

typedef struct{
	__vo uint32_t CPUID; 				
	__vo uint32_t ICSR; 				
	__vo uint32_t VTOR; 				
	__vo uint32_t AIRCR; 				
	__vo uint32_t SCR; 					
	__vo uint32_t CCR;		 			
	__vo uint32_t SHPR1;				
	__vo uint32_t SHPR2;				
	__vo uint32_t SHPR3;				
	__vo uint32_t SHCSR;				
	__vo uint32_t CFSR;					
	__vo uint32_t HFSR;					
	__vo uint32_t DFSR;					
	__vo uint32_t MMFAR;				
	__vo uint32_t BFAR;					
	__vo uint32_t AFSR;					
} SCB_TypdeDef_t;

#define NVIC				((NVIC_TypeDef_t*)NVIC_BASE_ADDR)
#define SCB					((SCB_TypdeDef_t*)SCB_BASEADDR)

/*
 * IQR configuring and handling
 */

void NVIC_InterruptConfig(uint8_t IRQNumber, uint8_t EnorDi);

#endif /* INC_STM32F103XX_CORE_DRIVER_H_ */
