/*
 * stm32f103xx_gpio_drivers.c
 *
 *  Created on: Jun 11, 2025
 *      Author: nphuc
 */

#include "stm32f103xx_gpio_driver.h"

void GPIO_PeriClockControl(GPIO_TypeDef_t *pGPIOx, uint8_t EnorDi)
{
	if (EnorDi == ENABLE)
	{
		if (pGPIOx == GPIOA)
		{
			GPIOA_PCLK_EN();
		}
	}
}

static void AlternativeMode_Init(GPIO_Handle_t *pGPIOHandle, uint8_t posPinNumber, uint32_t *reg)
{
	/* configure the alt functionality */
	switch (pGPIOHandle->GPIO_PinConfig.GPIO_PinAltFunMode)
	{
	case GPIO_ALT_MODE_OUT_PP:
		/* configure the pin as a output */
		uint8_t speed = pGPIOHandle->GPIO_PinConfig.GPIO_PinSpeed == 0 ? 1 : pGPIOHandle->GPIO_PinConfig.GPIO_PinSpeed;
		(*reg) |= (speed << (4 * posPinNumber));

		/* configure alternative push pull */
		(*reg) |= (GPIO_CFG_OUT_AL_PP << (4 * posPinNumber + 2));
		break;
		break;
	case GPIO_ALT_MODE_IN_FLOATING:
		/* configure the pin as a input */
		(*reg) |= (GPIO_MODE_IN << (4 * posPinNumber));

		/* configure input floating */
		(*reg) |= (GPIO_CFG_IN_FLOATING << (4 * posPinNumber + 2));
		break;
	default:
		break;
	}
}

void GPIO_Init(GPIO_Handle_t *pGPIOHandle)
{
	uint32_t temp = 0;
	uint8_t posPinNumber = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber % 8;
	uint8_t posReg = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber / 8;

	/* enable the peripheral clock */
	GPIO_PeriClockControl(pGPIOHandle->pGPIOx, ENABLE);

	/* 1. configure the mode of gpio pin and speed */
	switch (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode)
	{
	case GPIO_MODE_IN ... GPIO_MODE_OUT:
		/* configure the speed */
		temp |= (pGPIOHandle->GPIO_PinConfig.GPIO_PinSpeed << (4 * posPinNumber));
		temp |= (pGPIOHandle->GPIO_PinConfig.GPIO_PinCfgMode << (4 * posPinNumber + 2));
		pGPIOHandle->pGPIOx->CR[posReg] &= ~(3 << (4 * posPinNumber));
		pGPIOHandle->pGPIOx->CR[posReg] &= ~(3 << (4 * posPinNumber + 2));
		/* configure control function (output open drain/push pull, input floating/pull up/pull down) */
		pGPIOHandle->pGPIOx->CR[posReg] |= temp;
		break;
	case GPIO_MODE_ALTFN:
		AlternativeMode_Init(pGPIOHandle, posPinNumber, &temp);

		pGPIOHandle->pGPIOx->CR[posReg] &= ~(3 << (4 * posPinNumber));
		pGPIOHandle->pGPIOx->CR[posReg] &= ~(3 << (4 * posPinNumber + 2));

		pGPIOHandle->pGPIOx->CR[posReg] |= temp;
		break;
	}

	temp = 0;

	/* configure pull up/pull down */
	temp |= (pGPIOHandle->GPIO_PinConfig.GPIO_PinPuPdControl << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);

	pGPIOHandle->pGPIOx->ODR &= ~(1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
	pGPIOHandle->pGPIOx->ODR |= temp;
}

void GPIO_Toggle(GPIO_TypeDef_t *pGPIOx, uint8_t PinNumber)
{
	pGPIOx->ODR ^= (1 << PinNumber);
}
