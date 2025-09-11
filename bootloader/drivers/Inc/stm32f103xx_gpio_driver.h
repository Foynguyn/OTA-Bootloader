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
#define GPIOB									((GPIO_TypeDef_t*)GPIOB_BASEADDR)
#define GPIOC									((GPIO_TypeDef_t*)GPIOC_BASEADDR)
#define GPIOD									((GPIO_TypeDef_t*)GPIOD_BASEADDR)
#define GPIOE									((GPIO_TypeDef_t*)GPIOE_BASEADDR)
#define GPIOF									((GPIO_TypeDef_t*)GPIOF_BASEADDR)
#define GPIOG									((GPIO_TypeDef_t*)GPIOG_BASEADDR)

/*
 * Clock enable macros for GPIOx peripheral
 */

#define GPIOA_PCLK_EN()							(RCC->APB2ENR |= (1 << 2))
#define GPIOB_PCLK_EN()							(RCC->APB2ENR |= (1 << 3))
#define GPIOC_PCLK_EN()							(RCC->APB2ENR |= (1 << 4))
#define GPIOD_PCLK_EN()							(RCC->APB2ENR |= (1 << 5))
#define GPIOE_PCLK_EN()							(RCC->APB2ENR |= (1 << 6))
#define GPIOF_PCLK_EN()							(RCC->APB2ENR |= (1 << 7))
#define GPIOG_PCLK_EN()							(RCC->APB2ENR |= (1 << 8))

/*
 * Clock disable macros for GPIOx peripheral
 */

#define GPIOA_PCLK_DI()							RCC->APB2ENR &= ~(1 << 2)
#define GPIOB_PCLK_DI()							RCC->APB2ENR &= ~(1 << 3)
#define GPIOC_PCLK_DI()							RCC->APB2ENR &= ~(1 << 4)
#define GPIOF_PCLK_DI()							RCC->ABB2ENR &= ~(1 << 7)
#define GPIOD_PCLK_DI()							RCC->ABB2ENR &= ~(1 << 5)
#define GPIOE_PCLK_DI()							RCC->ABB2ENR &= ~(1 << 6)
#define GPIOG_PCLK_DI()							RCC->ABB2ENR &= ~(1 << 8)

/*
 * @GPIO_PIN_NUMBERS
 * GPIO pin numbers
 */

#define GPIO_PIN_NO_0			  				0
#define GPIO_PIN_NO_1			  				1
#define GPIO_PIN_NO_2			  				2
#define GPIO_PIN_NO_3			  				3
#define GPIO_PIN_NO_4			  				4
#define GPIO_PIN_NO_5			  				5
#define GPIO_PIN_NO_6			  				6
#define GPIO_PIN_NO_7			  				7
#define GPIO_PIN_NO_8			  				8
#define GPIO_PIN_NO_9			  				9
#define GPIO_PIN_NO_10  						10
#define GPIO_PIN_NO_11 							11
#define GPIO_PIN_NO_12  						12
#define GPIO_PIN_NO_13 							13
#define GPIO_PIN_NO_14 							14
#define GPIO_PIN_NO_15			 				15

/*
 * GPIO pin posible modes
 */
#define GPIO_MODE_IN 							0
#define GPIO_MODE_OUT							1

#define GPIO_SPEED_SLOW							GPIO_MODE_OUT
#define GPIO_SPEED_MEDIUM						2
#define GPIO_SPEED_FAST							3

#define GPIO_MODE_IT_FT							4
#define GPIO_MODE_IT_RT							5
#define GPIO_MODE_IT_RFT						6
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
#define GPIO_CFG_IN_ANALOG 						0
#define GPIO_CFG_IN_FLOATING 					1
#define GPIO_CFG_IN_PUPD 						2

#define GPIO_PULLUP								1
#define GPIO_PULLDOWN							0
/*
 * alternative mode case
 */

#define GPIO_ALT_MODE_OUT_PP					0
#define GPIO_ALT_MODE_OUT_OD					1

#define GPIO_ALT_MODE_IN_FLOATING 				2
#define GPIO_ALT_MODE_IN_PUPD 					3

#define NOTUSED									-1

/* UART */
/* TX */
#define GPIO_ALT_MODE_USART_TX_FULLDUP			GPIO_ALT_MODE_OUT_PP
#define GPIO_ALT_MODE_USART_TX_HALFDUP_SYNC		GPIO_ALT_MODE_OUT_PP

/* RX */
#define GPIO_ALT_MODE_USART_RX_FULLDUP			GPIO_ALT_MODE_IN_FLOATING
#define GPIO_ALT_MODE_USART_RX_HALFDUP_SYNC		NOTUSED

/* CK */
#define GPIO_ALT_MODE_USART_CK_SYNC				GPIO_ALT_MODE_OUT_PP
/* RTS */
#define GPIO_ALT_MODE_USART_RTS_HW				GPIO_ALT_MODE_OUT_PP
/* CTS */
#define GPIO_ALT_MODE_USART_CTS_HW				GPIO_ALT_MODE_IN_FLOATING

/* SPI
SCK
#define GPIO_ALT_MODE_SPI_SCLK_MASTER			GPIO_ALT_MODE_OUT_PP
#define GPIO_ALT_MODE_SPI_SCLK_SLAVE			GPIO_ALT_MODE_IN_FLOATING

MOSI
#define GPIO_ALT_MODE_SPI_MOSI_MASTER			GPIO_ALT_MODE_OUT_PP
#define GPIO_ALT_MODE_SPI_MOSI_SLAVE			GPIO_ALT_MODE_IN_FLOATING
#define GPIO_ALT_MODE_SPI_MOSI_SIMP_MASTER		GPIO_ALT_MODE_OUT_PP
#define GPIO_ALT_MODE_SPI_MOSI_SIMP_SLAVE		NOTUSED

MISO
#define GPIO_ALT_MODE_SPI_MISO_MASTER			GPIO_ALT_MODE_IN_FLOATING
#define GPIO_ALT_MODE_SPI_MISO_P2P_SLAVE		GPIO_ALT_MODE_OUT_PP
#define GPIO_ALT_MODE_SPI_MISO_MULTI_SLAVE		GPIO_ALT_MODE_OUT_OD
#define GPIO_ALT_MODE_SPI_MISO_P2P_SIMP_MASTER	NOTUSED
#define GPIO_ALT_MODE_SPI_MISO_P2P_SIMP_SLAVE	GPIO_ALT_MODE_OUT_PP
#define GPIO_ALT_MODE_SPI_MISO_MULTI_SIMP_SLAVE	GPIO_ALT_MODE_OUT_OD

NSS
#define GPIO_ALT_MODE_SPI_NSS_SLAVE				GPIO_ALT_MODE_OUT_PP
#define GPIO_ALT_MODE_SPI_NSS_MASTER			NOTUSED */


/* I2S */


/*
 * APIs support prototype for GPIO
 */


/*
 * Macro to reset GPIO peripherals
 */

#define GPIOA_REG_RESET()	do{(RCC->APB2RSTR |= 1 << 2);	(RCC->APB2RSTR &= ~(1 << 2));} while(0)
#define GPIOB_REG_RESET()	do{(RCC->APB2RSTR |= 1 << 3);	(RCC->APB2RSTR &= ~(1 << 3));} while(0)
#define GPIOC_REG_RESET()	do{(RCC->APB2RSTR |= 1 << 4);	(RCC->APB2RSTR &= ~(1 << 4));} while(0)
#define GPIOD_REG_RESET()	do{(RCC->APB2RSTR |= 1 << 5);	(RCC->APB2RSTR &= ~(1 << 5));} while(0)
#define GPIOE_REG_RESET()	do{(RCC->APB2RSTR |= 1 << 6);	(RCC->APB2RSTR &= ~(1 << 6));} while(0)
#define GPIOF_REG_RESET()	do{(RCC->APB2RSTR |= 1 << 7);	(RCC->APB2RSTR &= ~(1 << 7));} while(0)
#define GPIOG_REG_RESET()	do{(RCC->APB2RSTR |= 1 << 8);	(RCC->APB2RSTR &= ~(1 << 8));} while(0)

#define GPIO_BASEADDR_TO_CODE(x)	\
	((x == GPIOA) ? 0 : \
	(x == GPIOB) ? 1 : \
	(x == GPIOC) ? 2 : \
	(x == GPIOD) ? 3 : \
	(x == GPIOE) ? 4 : \
	(x == GPIOG) ? 5 : 0xFF)

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
 * Init and De-Init
 */

void GPIO_Init(GPIO_Handle_t *pGPIOHandle);
void GPIO_DeInit(GPIO_TypeDef_t *pGPIOx);

/*
 * Data read and write
 */

uint8_t GPIO_ReadPin(GPIO_TypeDef_t *pGPIOx, uint8_t PinNumber);
void GPIO_WritePin(GPIO_TypeDef_t *pGPIOx, uint8_t PinNumber, uint8_t Value);
void GPIO_Toggle(GPIO_TypeDef_t *pGPIOx, uint8_t PinNumber);

void GPIO_IRQHandling(uint8_t PinNumber); // call when IRQ occur

__weak void GPIO_ExternalInterruptEventsCallback(uint8_t PinNumber);

#endif /* INC_STM32F103XX_GPIO_DRIVER_H_ */
