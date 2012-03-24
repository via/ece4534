/**************************************************************************//**
 * @file     lpc17xx_rtc.c
 * @brief    Drivers for RTC peripheral in lpc17xx.
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

#include "LPC17xx.H"                              /* LPC17xx definitions    */
#include "lpc17xx_rtc.h"

#define CCR_CLKEN	0x01
#define CCR_CTCRST	0x02

/* Initialize RTC timer */
void LPC17xx_RTC_Init (void)
{
	/* Enable CLOCK into RTC */
	LPC_SC->PCONP |= (1 << 9);
	
	/* If RTC is stopped, clear STOP bit. */
	if ( LPC_RTC->RTC_AUX & (0x1<<4) )
	{
		LPC_RTC->RTC_AUX |= (0x1<<4);	
	}
	
	/* Initialize registers */    
	LPC_RTC->AMR = 0;
	LPC_RTC->CIIR = 0;
	LPC_RTC->CCR = 0;
}

/* Start RTC timer */
void LPC17xx_RTC_Start( void ) 
{
	/* Start RTC counters */
	LPC_RTC->CCR |= CCR_CLKEN;
}

/* Stop RTC timer */
void LPC17xx_RTC_Stop( void )
{   
	/* Stop RTC counters */
	LPC_RTC->CCR &= ~CCR_CLKEN;
}

/* Reset RTC clock tick counter */
void LPC17xx_RTC_CTCReset( void )
{   
	/* Reset CTC */
	LPC_RTC->CCR |= CCR_CTCRST;
}

/* Setup RTC timer value */
void LPC17xx_RTC_SetTime( const RTCTime *rtc ) 
{
	LPC_RTC->SEC    = rtc->sec;
	LPC_RTC->MIN    = rtc->min;
	LPC_RTC->HOUR   = rtc->hour;
	LPC_RTC->DOM    = rtc->mday;
	LPC_RTC->DOW    = rtc->wday;
	LPC_RTC->DOY    = rtc->yday;
	LPC_RTC->MONTH  = rtc->mon;
	LPC_RTC->YEAR   = rtc->year;    
}

/* Get RTC timer value */
void LPC17xx_RTC_GetTime( RTCTime *rtc ) 
{
	rtc->sec    = LPC_RTC->SEC;
	rtc->min    = LPC_RTC->MIN;
	rtc->hour   = LPC_RTC->HOUR;
	rtc->mday   = LPC_RTC->DOM;
	rtc->wday   = LPC_RTC->DOW;
	rtc->yday   = LPC_RTC->DOY;
	rtc->mon    = LPC_RTC->MONTH;
	rtc->year   = LPC_RTC->YEAR;   
}

/* --------------------------------- End Of File ------------------------------ */

