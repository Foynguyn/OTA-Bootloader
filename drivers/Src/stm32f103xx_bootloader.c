/*
 * stm32f103xx_bootloader.c
 *
 *  Created on: Sep 11, 2025
 *      Author: nphuc
 */


#include "stm32f103xx_bootloader.h"



void Bootloader_Reset(){
    /* Memory barriers */
    __asm volatile ("dsb sy");

    /* Ghi thanh ghi */
    SCB->AIRCR = (0x5FA << AIRCR_VECTKEY) |
                (1 << AIRCR_SYSRESETREQ);

    __asm volatile ("dsb sy");
}

void Bootloader_JumpApp(uint32_t appAddress){
	__asm volatile ("cpsid i");

	uint32_t appStack = *(volatile uint32_t*)appAddress;
	void (*appFunction)() = (void (*)())*(uint32_t*)(appAddress + 4);

	__asm volatile ("MSR msp, %0" : : "r" (appStack) : );

	SCB->VTOR = appAddress;

	__asm volatile ("cpsie i");

	appFunction();
}

uint8_t Bootloader_CheckApp(uint32_t appAddress, uint32_t appAddressEnd){
	uint32_t app_stack = *(volatile uint32_t*)appAddress;
	uint32_t reset_handler = *(volatile uint32_t*)(appAddress + 4);
	if((app_stack >= 0x20000000 && app_stack <= 0x20005000) &&
		   (reset_handler >= appAddress && reset_handler <= appAddressEnd)) {
			return 1;
		}
	return 0;
}
