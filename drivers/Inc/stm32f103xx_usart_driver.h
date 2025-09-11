/*
 * stm32f103xx_uart_driver.h
 *
 *  Created on: Aug 1, 2025
 *      Author: nphuc
 */

#ifndef INC_STM32F103XX_USART_DRIVER_H_
#define INC_STM32F103XX_USART_DRIVER_H_

#include "stm32f103xx.h"

#define USART1								((USART_TypeDef_t*)USART1_BASEADDR)

/*
 *@USART_Mode
 *Possible options for USART_Mode
 */

#define USART_MODE_ONLY_TX 					0
#define USART_MODE_ONLY_RX 					1
#define USART_MODE_TXRX  					2

/*
 *@USART_Baud
 *Possible options for USART_Baud
 */
#define USART_STD_BAUD_1200					1200UL
#define USART_STD_BAUD_2400					400UL
#define USART_STD_BAUD_9600					9600UL
#define USART_STD_BAUD_19200 				19200UL
#define USART_STD_BAUD_38400 				38400UL
#define USART_STD_BAUD_57600 				57600UL
#define USART_STD_BAUD_115200 				115200UL
#define USART_STD_BAUD_230400 				230400UL
#define USART_STD_BAUD_460800 				460800UL
#define USART_STD_BAUD_921600 				921600UL
#define USART_STD_BAUD_2M 					2250000UL
#define SUART_STD_BAUD_3M 					4500000UL

/*
 *@USART_ParityControl
 *Possible options for USART_ParityControl
 */
#define USART_PARITY_EN_ODD   				2
#define USART_PARITY_EN_EVEN  				1
#define USART_PARITY_DISABLE   				0

/*
 *@USART_WordLength
 *Possible options for USART_WordLength
 */
#define USART_WORDLEN_8BITS  				0
#define USART_WORDLEN_9BITS  				1

/*
 *@USART_NoOfStopBits
 *Possible options for USART_NoOfStopBits
 */
#define USART_STOPBITS_1     				0
#define USART_STOPBITS_0_5   				1
#define USART_STOPBITS_2     				2
#define USART_STOPBITS_1_5   				3

/*
 *@USART_HWFlowControl
 *Possible options for USART_HWFlowControl
 */

#define USART_HW_FLOW_CTRL_NONE    			0
#define USART_HW_FLOW_CTRL_CTS    			1
#define USART_HW_FLOW_CTRL_RTS    			2
#define USART_HW_FLOW_CTRL_CTS_RTS			3

/*
 * Clock enable macros for USARTx peripheral
 */
#define USART1_PCLK_EN()					RCC->APB2ENR |= 1 << 14

/* peripheral register definition structure for USART */
typedef struct{
	__vo uint32_t SR;
	__vo uint32_t DR;
	__vo uint32_t BRR;
	__vo uint32_t CR1;
	__vo uint32_t CR2;
	__vo uint32_t CR3;
	__vo uint32_t GTPR;
} USART_TypeDef_t;

typedef struct
{
	uint8_t USART_Mode;
	uint32_t USART_Baudrate;
	uint8_t USART_NumberOfStopBits;
	uint8_t USART_WordLength;
	uint8_t USART_ParityControl;
	uint8_t USART_HWFLowControl;
} USART_Config_t;

typedef struct
{
	USART_TypeDef_t *pUSARTx;
	USART_Config_t USART_Config;
	uint8_t *pTxBuffer;
	uint8_t *pRxBuffer;
	uint32_t TxLength;
	uint32_t RxLength;
	uint8_t TxState;
	uint8_t RxState;
} USART_Handle_t;

extern USART_Handle_t uart1;

/*
 * USART flags
 */

#define USART_CR1_SBK						0
#define USART_CR1_RWU						1
#define USART_CR1_RE						2
#define USART_CR1_TE						3
#define USART_CR1_IDLEIE					4
#define USART_CR1_RXNEIE					5
#define USART_CR1_TCIE						6
#define USART_CR1_TXEIE						7
#define USART_CR1_PEIE						8
#define USART_CR1_PS						9
#define USART_CR1_PCE						10
#define USART_CR1_WAKE						11
#define USART_CR1_M							12
#define USART_CR1_UE						13

#define USART_CR2_ADD						0
#define USART_CR2_LBDL						5
#define USART_CR2_LBDIE						6
#define USART_CR2_LBCL						8
#define USART_CR2_CPHA						9
#define USART_CR2_CPOL						10
#define USART_CR2_CLKEN						11
#define USART_CR2_STOP						12
#define USART_CR2_LINEN						14

#define USART_CR3_EIE						0
#define USART_CR3_IREN						1
#define USART_CR3_IRLP						2
#define USART_CR3_HDSEL						3
#define USART_CR3_NACK						4
#define USART_CR3_SCEN						5
#define USART_CR3_DMAR						6
#define USART_CR3_DMAT						7
#define USART_CR3_RTSE						8
#define USART_CR3_CTSE						9
#define USART_CR3_CTSIE						10

#define USART_SR_PE							0
#define USART_SR_FE							1
#define USART_SR_NE							2
#define USART_SR_ORE						3
#define USART_SR_IDLE						4
#define USART_SR_RXNE						5
#define USART_SR_TC							6
#define USART_SR_TXE						7
#define USART_SR_LBD						8
#define USART_SR_CTS						9

/*
 * Application states
 */
#define USART_BUSY_RX 						1
#define USART_BUSY_TX 						2
#define USART_READY 						0


void USART_PeriClockControl(USART_TypeDef_t *pUSARTx, uint8_t EnorDi);

void USART_Start(USART_TypeDef_t *pUSARTx);

void USART_Init(USART_Handle_t *pUSARTHandle);


void USART_SendData(USART_Handle_t *pUSARTHandle, uint8_t *pTxBuffer, uint32_t length);

void USART_SetBaudRate(USART_TypeDef_t *pUSARTx, uint32_t BaudRate);

uint8_t USART_ReceiveDataIT(USART_Handle_t *pUSARTHandle,uint8_t *pRxBuffer, uint32_t length);

/*
 * IRQ Configuation and ISR Handling
 */

void USART_IRQHandling(USART_Handle_t *pUSARTHandle);

__weak void USART_ReceptionEventsCallback(USART_Handle_t *pUSARTHandle);

#endif /* INC_STM32F103XX_USART_DRIVER_H_ */
