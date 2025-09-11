/*
 * stm32f103xx_interrupt_driver.c
 *
 *  Created on: Aug 5, 2025
 *      Author: nphuc
 */


#include <stm32f103xx_core_driver.h>

/*
 * IQR configuring and handling
 */

/* refered from cortex M3 devices generic user guide */
void NVIC_InterruptConfig(uint8_t IRQNumber, uint8_t EnorDi){ // config IRQ number
	if(EnorDi){
		NVIC->ISER[IRQNumber/32] |= (1 << (IRQNumber % 32));
	}else{
		NVIC->ICER[IRQNumber/32] |= (1 << (IRQNumber % 32));
	}
}
