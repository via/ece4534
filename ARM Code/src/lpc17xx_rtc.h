/**************************************************************************//**
 * @file     lpc17xx_rtc.h
 * @brief    Header file for lpc17xx_rtc.c.
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

#ifndef __LPC17xx_RTC_H
#define __LPC17xx_RTC_H

/* RTC timer strcuture */
typedef struct {
    uint8_t     sec;     /* Second value - [0,59] */
    uint8_t     min;     /* Minute value - [0,59] */
    uint8_t     hour;    /* Hour value - [0,23] */
    uint8_t     mday;    /* Day of the month value - [1,31] */
	uint8_t     wday;    /* Day of week value - [0,6] */
	uint16_t    yday;    /* Day of year value - [1,365] */
    uint8_t     mon;     /* Month value - [1,12] */
    uint16_t    year;    /* Year value - [0,4095] */    
} RTCTime;

/* public functions */
void LPC17xx_RTC_Init( void );
void LPC17xx_RTC_Start( void );
void LPC17xx_RTC_Stop( void );
void LPC17xx_RTC_CTCReset( void );
void LPC17xx_RTC_SetTime( const RTCTime *);
void LPC17xx_RTC_GetTime( RTCTime *);


#endif /* end __LPC17xx_RTC_H */

/* --------------------------------- End Of File ------------------------------ */
