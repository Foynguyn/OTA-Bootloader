/*
 * stm32f103xx_gpio_drivers.h
 *
 *  Created on: Jun 11, 2025
 *      Author: nphuc
 */

#ifndef INC_STM32F103XX_GPIO_DRIVER_H_
#define INC_STM32F103XX_GPIO_DRIVER_H_

#include "stm32f103xx.h"

#define GPIOA									((GPIO_TypeDef_t*)GPIOA_BASEADDR)

/*
 * Clock enable macros for GPIOx peripheral
 */

#define GPIOA_PCLK_EN()							(RCC->APB2ENR |= (1 << 2))

/*
 * @GPIO_PIN_NUMBERS
 * GPIO pin numbers
 */

#define GPIO_PIN_NO_9			  				9
#define GPIO_PIN_NO_10  						10

/*
 * GPIO pin posible modes
 */
#define GPIO_MODE_IN 							0
#define GPIO_MODE_OUT							1

#define GPIO_SPEED_SLOW							GPIO_MODE_OUT
#define GPIO_SPEED_MEDIUM						2
#define GPIO_SPEED_FAST							3

#define GPIO_MODE_ALTFN							7

/*
 * GPIO pin possible output type
 */

#define GPIO_CFG_OUT_GE_PP 						0
#define GPIO_CFG_OUT_GE_OD 						1
#define GPIO_CFG_OUT_AL_PP 						2
#define GPIO_CFG_OUT_AL_OD 						3

/*
 * GPIO pin possible input type
 */
#define GPIO_CFG_IN_FLOATING 					1
#define GPIO_CFG_IN_PUPD 						2

/*
 * alternative mode case
 */

#define GPIO_ALT_MODE_OUT_PP					0
//#define GPIO_ALT_MODE_OUT_OD					1

#define GPIO_ALT_MODE_IN_FLOATING 				2
//#define GPIO_ALT_MODE_IN_PUPD 					3

#define NOTUSED									-1

/* UART */
/* TX */
#define GPIO_ALT_MODE_USART_TX_FULLDUP			GPIO_ALT_MODE_OUT_PP

/* RX */
#define GPIO_ALT_MODE_USART_RX_FULLDUP			GPIO_ALT_MODE_IN_FLOATING

/* peripheral register definition structure for GPIO */
typedef struct{
	__vo uint32_t CR[2];
	__vo uint32_t IDR;
	__vo uint32_t ODR;
	__vo uint32_t BSRR;
	__vo uint32_t BRR;
	__vo uint32_t LCKR;
} GPIO_TypeDef_t;

typedef struct
{
	uint8_t GPIO_PinNumber;
	uint8_t GPIO_PinMode;
	uint8_t GPIO_PinCfgMode;
	uint8_t GPIO_PinSpeed;
	uint8_t GPIO_PinPuPdControl;
	uint8_t GPIO_PinAltFunMode;
} GPIO_PinConfig_t;

typedef struct
{
	GPIO_TypeDef_t *pGPIOx;
	GPIO_PinConfig_t GPIO_PinConfig;
} GPIO_Handle_t;

/*
 * Peripheral clock setup
 */

void GPIO_PeriClockControl(GPIO_TypeDef_t *pGPIOx, uint8_t EnorDi);

/*
 * Init
 */

void GPIO_Init(GPIO_Handle_t *pGPIOHandle);

#endif /* INC_STM32F103XX_GPIO_DRIVER_H_ */
