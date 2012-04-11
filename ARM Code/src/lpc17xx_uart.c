/**************************************************************************//**
 * @file     lpc17xx_uart.c
 * @brief    Drivers for UART peripheral in lpc17xx.
 * @version  1.0
 * @date     18. Nov. 2010
 *
 * @note
 * Copyright (C) 2010 NXP Semiconductors(NXP). All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
 ******************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include "lpc17xx_uart.h"
#include "lpc17xx.h"


/**
  * @brief  Initializes the UART0.
  *
  * @param  baudrate: Specifies the baud rate
  * @retval None 
  */
void  LPC17xx_UART_Init(uint32_t baudrate)
{
    uint32_t  Fdiv;
	uint32_t  pclk;

    LPC_PINCON->PINSEL0 |= 0x00000050;       /* RxD0 and TxD0 */    

	/* PCLK_UART0=CCLK/2 */
	LPC_SC->PCLKSEL1 &= ~(3<<6);               /* PCLK_UART0 = CCLK/4 (18MHz) */
	LPC_SC->PCLKSEL1 |=  (2<<6);               /* PCLK_UART0 = CCLK/2   (36MHz) */

	pclk = SystemCoreClock/2;
	
    LPC_UART0->LCR = 0x83;		/* 8 bits, no Parity, 1 Stop bit */
	Fdiv = ( pclk / 16 ) / baudrate ;	/*baud rate */
    LPC_UART0->DLM = Fdiv / 256;							
    LPC_UART0->DLL = Fdiv % 256;
    LPC_UART0->LCR = 0x03;		/* DLAB = 0 */
    LPC_UART0->FCR = 0x07;		/* Enable and reset TX and RX FIFO. */
}

/**
  * @brief  Write one character to UART0.
  *
  * @param  ch: Character to be written
  * @retval None 
  */
void LPC17xx_UART_PutChar (uint8_t ch) 
{
	while (!(LPC_UART0->LSR & 0x20));
    	
	LPC_UART0->THR = ch;
}

/**
  * @brief  Read one character from UART0 (blocking read).
  *
  * @param  None
  * @retval Received character 
  */
uint8_t LPC17xx_UART_GetChar (void) 
{
	while (!(LPC_UART0->LSR & 0x01));
	return (LPC_UART0->RBR);
}

/**
  * @brief  Read one character from UART0 (non blocking read).
  *
  * @param  None
  * @retval Received character 
  */
uint8_t LPC17xx_UART_GetChar_nb (void) 
{
	if (LPC_UART0->LSR & 0x01)
		return (LPC_UART0->RBR);
	else
		return 0;
}

/**
  * @brief  Write a string to UART0.
  *
  * @param  str: NULL-terminated char string to be written
  * @retval None 
  */
void LPC17xx_UART_PutString (uint8_t *str) 
{
/* usage: LPC1700_UART_Printf("xxx\n\r");*/
#if 1
	while (*str != 0) 
	{
		LPC17xx_UART_PutChar(*str++);
	}

#else
/* usage: LPC1700_UART_Printf("xxx\n");*/
   while ((*str) != 0) {
      if (*str == '\n') {
         LPC17xx_UART_PutChar(*str++);
         LPC17xx_UART_PutChar('\r');
      } else {
         LPC17xx_UART_PutChar(*str++);
      }    
   }
#endif
}

/**
  * @brief  Print formatted string. This function takes variable length arguments.
  *
  * @param  format
  * @param  ...
  * @retval None 
  *
  * Note: using library functions "vsprintf" will increase the RO size by about 6KB
  */
void  LPC17xx_UART_Printf (const  uint8_t *format, ...)
{
    static  uint8_t  buffer[40 + 1];
    va_list     vArgs;

    va_start(vArgs, format);
    vsprintf((char *)buffer, (char const *)format, vArgs);
    va_end(vArgs);
    LPC17xx_UART_PutString((uint8_t *) buffer);
}

/* --------------------------------- End Of File ------------------------------ */
