#ifndef _STM32F1XX_RCC_H_
#define _STM32F1XX_RCC_H_

#include "stm32f103xx.h"

/* ------------------------ TYPE DEFINITIONS ------------------------ */
/* peripheral register definition structure for RCC */
typedef struct{
	__vo uint32_t CR;
	__vo uint32_t CFGR;
	__vo uint32_t CIR;
	__vo uint32_t APB2RSTR;
	__vo uint32_t APB1RSTR;
	__vo uint32_t AHBENR;
	__vo uint32_t APB2ENR;
	__vo uint32_t APB1ENR;
	__vo uint32_t BDCR;
	__vo uint32_t CSR;
	__vo uint32_t AHBSTR;
	__vo uint32_t CFGR2;
} RCC_TypeDef_t;

#define RCC			((RCC_TypeDef_t*)(RCC_BASEADDR))

#endif
