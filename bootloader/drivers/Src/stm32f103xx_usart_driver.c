/*
 * stm32f103xx_uart.c
 *
 *  Created on: Aug 1, 2025
 *      Author: nphuc
 */

#include "stm32f103xx_usart_driver.h"

static void USART_RXNE_Interrupt_Handle(USART_Handle_t *pUSARTHandle)
{
	/* Check are we using USART_ParityControl control or not */
	if (pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BITS)
	{
		/* check for USART_ParityControl */
		if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
		{
			/* No parity is used, 9bits will be of user data */
			*((uint16_t *)pUSARTHandle->pRxBuffer) = pUSARTHandle->pUSARTx->DR & (uint16_t)0x01FF;

			pUSARTHandle->pRxBuffer += 2;
			pUSARTHandle->RxLength -= 2;
		}
		else
		{
			/* Parity is used, 8 bits will be user data and 1 bit is parity */
			*pUSARTHandle->pRxBuffer = pUSARTHandle->pUSARTx->DR & 0xFF;

			pUSARTHandle->pRxBuffer++;
			pUSARTHandle->RxLength -= 1;
		}
	}
	else
	{
		if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
		{
			/* No parity is used, 8 bits will be of user data */
			*pUSARTHandle->pRxBuffer = (uint8_t)(pUSARTHandle->pUSARTx->DR & (uint8_t)0xFF);
		}
		else
		{
			/* Parity is used, 7 bits will be user data and 1 bit is parity */
			*pUSARTHandle->pRxBuffer = (uint8_t)(pUSARTHandle->pUSARTx->DR & (uint8_t)0x7F);
		}
		pUSARTHandle->pRxBuffer++;
		pUSARTHandle->RxLength -= 1;
	}

	if (!pUSARTHandle->RxLength)
	{
		pUSARTHandle->pUSARTx->CR1 &= ~(1 << USART_CR1_RXNEIE);

		pUSARTHandle->RxState = USART_READY;
		pUSARTHandle->pRxBuffer = NULL;
		pUSARTHandle->RxLength = 0;

		USART_ReceptionEventsCallback(pUSARTHandle);
	}
}

/*
 * Peripheral clock setup
 */
void USART_PeriClockControl(USART_TypeDef_t *pUSARTx, uint8_t EnorDi)
{
	if (!EnorDi) return;
	if (pUSARTx == USART1)
		USART1_PCLK_EN();
}

void USART_Start(USART_TypeDef_t *pUSARTx)
{
	pUSARTx->CR1 |= (1 << USART_CR1_UE);
}

/*
 * Init and De-Init
 */

void USART_Init(USART_Handle_t *pUSARTHandle)
{
	/* enable clock for USART */
	USART_PeriClockControl(pUSARTHandle->pUSARTx, ENABLE);
	uint32_t reg = 0;
	/* enable USART Tx and Rx engines according to the USART Mode configuration item */
	switch (pUSARTHandle->USART_Config.USART_Mode){
	case USART_MODE_ONLY_RX:
		reg |= (1 << USART_CR1_RE);
		break;
	case USART_MODE_ONLY_TX:
		reg |= (1 << USART_CR1_TE);
		break;
	case USART_MODE_TXRX:
		reg |= (1 << USART_CR1_TE) | (1 << USART_CR1_RE);
		break;
	default:
	}
	/* configure the word length */
	reg |= (pUSARTHandle->USART_Config.USART_WordLength << USART_CR1_M);
	/* configure parity bit */
	switch (pUSARTHandle->USART_Config.USART_ParityControl){
	case USART_PARITY_DISABLE:
		reg &= ~(1 << USART_CR1_PCE);
		break;
	case USART_PARITY_EN_EVEN:
		reg |= (1 << USART_CR1_PCE);
		break;
	case USART_PARITY_EN_ODD:
		reg |= (1 << USART_CR1_PCE) | (1 << USART_CR1_PS);
		break;
	default:
	}

	pUSARTHandle->pUSARTx->CR1 = reg;

	reg = 0;
	/* configure the number of stop bit */
	reg |= (pUSARTHandle->USART_Config.USART_NumberOfStopBits << USART_CR2_STOP);

	pUSARTHandle->pUSARTx->CR2 = reg;

	reg = 0;

	/* configure hardware flow control */
	switch (pUSARTHandle->USART_Config.USART_HWFLowControl)
	{
	case USART_HW_FLOW_CTRL_NONE:
		reg &= ~(1 << USART_CR3_CTSE);
		reg &= ~(1 << USART_CR3_RTSE);
		break;
	case USART_HW_FLOW_CTRL_CTS:
		reg |= (1 << USART_CR3_CTSE);
		break;
	case USART_HW_FLOW_CTRL_RTS:
		reg |= (1 << USART_CR3_RTSE);
		break;
	case USART_HW_FLOW_CTRL_CTS_RTS:
		reg |= (1 << USART_CR3_CTSE);
		reg |= (1 << USART_CR3_RTSE);
		break;
	default:
	}
	pUSARTHandle->pUSARTx->CR3 = reg;

	/* Implement the code to configure the baud rate
	We will cover this in the lecture. No action required here */
	USART_SetBaudRate(pUSARTHandle->pUSARTx, pUSARTHandle->USART_Config.USART_Baudrate);
}

void USART_SetBaudRate(USART_TypeDef_t *pUSARTx, uint32_t BaudRate)
{
	/* APB clock */
	uint32_t PCLKx = 8000000;

	uint32_t usartdiv;

	uint32_t M_part, F_part;
	uint32_t reg = 0;

	usartdiv = (25 * PCLKx) / (4 * BaudRate);

	M_part = usartdiv / 100;
	reg |= M_part << 4;

	F_part = usartdiv % 100;

	F_part = (((F_part * 16) + 50) / 100) & 0x0F;

	reg |= F_part;

	pUSARTx->BRR = reg;
}

void USART_SendData(USART_Handle_t *pUSARTHandle, uint8_t *pTxBuffer, uint32_t length)
{
	while (length > 0)
	{
		/* wait until TXE flag is set in the SR */
		while (!((pUSARTHandle->pUSARTx->SR >> USART_SR_TXE) & 1));

		/* Check the USART_WordLength */
		if (pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BITS){
			/* check for USART_ParityControl */
			if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE){
				pUSARTHandle->pUSARTx->DR = (*((uint16_t *)pTxBuffer) & (uint16_t)0x01FF);
				/* 9 bits of user data will be sent */
				(uint16_t *)pTxBuffer++;
			}
			else{
				pUSARTHandle->pUSARTx->DR = (*(pTxBuffer) & (uint8_t)0xFF);
				/* Parity bit is used in this transfer . so 8bits of user data will be sent */
				pTxBuffer++;
			}
		}
		else{
			/* This is 8bit data transfer */
			pUSARTHandle->pUSARTx->DR = (*(pTxBuffer) & (uint8_t)0xFF);

			/* increment the transmit buffer address by 1 */
			pTxBuffer++;
		}
		length--;
	}
	/* wailt till TC flag is set in the SR */
	while (!((pUSARTHandle->pUSARTx->SR >> USART_SR_TC) & 1));
}

uint8_t USART_ReceiveDataIT(USART_Handle_t *pUSARTHandle, uint8_t *pRxBuffer, uint32_t length){
	uint8_t state = pUSARTHandle->RxState;
	if (state != USART_BUSY_RX){
		pUSARTHandle->pRxBuffer = pRxBuffer;
		pUSARTHandle->RxLength = length;

		pUSARTHandle->RxState = USART_BUSY_RX;

		pUSARTHandle->pUSARTx->CR1 |= (1 << USART_CR1_RXNEIE);
	}
	return state;
}

/*
 * IRQ Configuation and ISR Handling
 */

void USART_IRQHandling(USART_Handle_t *pUSARTHandle){
	uint8_t temp1, temp2;

	temp1 = (pUSARTHandle->pUSARTx->SR >> USART_SR_RXNE) & 1;
	temp2 = (pUSARTHandle->pUSARTx->CR1 >> USART_CR1_RXNEIE) & 1;

	if (temp1 && temp2)
		USART_RXNE_Interrupt_Handle(pUSARTHandle);
}

void USART1_IRQHandler()
{
	USART_IRQHandling(&uart1);
}

__weak void USART_ReceptionEventsCallback(USART_Handle_t *pUSARTHandle) {}
