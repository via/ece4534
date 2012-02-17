#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "semphr.h"

/* include files. */
#include "vtUtilities.h"
#include "vtI2C.h"
#include "LCDtask.h"
#include "i2cTemp.h"



// I have set this to a large stack size because of (a) using printf() and (b) the depth of function calls
//   for some of the LCD operations
#if PRINTF_VERSION==1
#define i2cSTACK_SIZE		8*configMINIMAL_STACK_SIZE
#else
#define i2cSTACK_SIZE		4*configMINIMAL_STACK_SIZE
#endif

// Set the task up to run every second (need to modify this to poll more often from the adc)
#define i2cREAD_RATE_BASE	( ( portTickType ) 1000)

/* The i2cTemp task. */
static portTASK_FUNCTION_PROTO( vi2cTempUpdateTask, pvParameters );

/*-----------------------------------------------------------*/

void vStarti2cTempTask( unsigned portBASE_TYPE uxPriority, i2cTempStruct *params)
{
	/* Start the task */
	portBASE_TYPE retval;

	if ((retval = xTaskCreate( vi2cTempUpdateTask, ( signed char * ) "i2cTemp", i2cSTACK_SIZE, (void *) params, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(retval);
	}
}

// This is the actual task that is run
static portTASK_FUNCTION( vi2cTempUpdateTask, pvParameters )
{
	portTickType xUpdateRate, xLastUpdateTime;

	const uint8_t i2cZero[] = {0x00};
	const uint8_t nmeaRead[] = {0x02};
	const uint8_t nmeaRead2[] = {0x03};
	uint8_t P0Read[2];
	uint8_t P1Read[2];
	uint8_t P2Read[2];
	uint8_t tempBuf[35];
	uint8_t *tempRead = tempBuf;
	uint8_t *tempRead10 = tempBuf + 9;
	uint8_t rxLen, status;
	// Get the parameters
	i2cTempStruct *param = (i2cTempStruct *) pvParameters;
	// Get the I2C device pointer
	vtI2CStruct *devPtr = param->dev;
	// Get the LCD information pointer
	vtLCDStruct *lcdData = param->lcdData;
	vtLCDMsg lcdBuffer;

	// Scale the update rate to ensure it really is in ms
	xUpdateRate = i2cREAD_RATE_BASE / portTICK_RATE_MS;

	/* We need to initialise xLastUpdateTime prior to the first call to vTaskDelayUntil(). */
	xLastUpdateTime = xTaskGetTickCount();
	
	// Like all good tasks, this should never exit
	for(;;)
	{
		/* Ask the RTOS to delay reschduling this task for the specified time */
		vTaskDelayUntil( &xLastUpdateTime, xUpdateRate );
		
		//PIC 0
		if (vtI2CEnQ(devPtr,0x00,0x1b,sizeof(i2cZero),i2cZero,2) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}

		if (vtI2CDeQ(devPtr,2,tempRead,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		
		//handle conversion of received here
		P0Read[0] = tempBuf[0];
		P0Read[1] = tempBuf[1];	
		vtITMu8(vtITMPortTempVals,rxLen); // Log the length received
		
		//PIC 1
		if (vtI2CEnQ(devPtr,0x00,0x1c,sizeof(i2cZero),i2cZero,2) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}

		if (vtI2CDeQ(devPtr,2,tempRead,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		
		//handle conversion of received here
		P1Read[0] = tempBuf[0];
		P1Read[1] = tempBuf[1];	
		vtITMu8(vtITMPortTempVals,rxLen); // Log the length received

		//PIC 2
		if (vtI2CEnQ(devPtr,0x00,0x1d,sizeof(i2cZero),i2cZero,2) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}

		if (vtI2CDeQ(devPtr,2,tempRead,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		
		//handle conversion of received here
		P2Read[0] = tempBuf[0];
		P2Read[1] = tempBuf[1];	
		vtITMu8(vtITMPortTempVals,rxLen); // Log the length received
		
		//NMEA String reads
		if (vtI2CEnQ(devPtr,0x00,0x1d,sizeof(nmeaRead),nmeaRead,10) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}

		if (vtI2CDeQ(devPtr,10,tempRead,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		
		vtITMu8(vtITMPortTempVals,rxLen); // Log the length received

		//Calculations and passing to LCD
		/*
		#if PRINTF_VERSION == 1
		//printf("Temp %f F (%f C)\n",(32.0 + ((9.0/5.0)*temperature)), (temperature));
		sprintf((char*)(lcdBuffer.buf),"%f", ADCReading);
		#else
		// we do not have full printf (so no %f) and therefore need to print out integers
		//printf("Temp %d F (%d C)\n",lrint(32.0 + ((9.0/5.0)*temperature)), lrint(temperature));
		
		sprintf((char*)(lcdBuffer.buf),"%d", ADCReading);
		#endif
		*/
		tempBuf[10]='\0';
		strncpy(lcdBuffer.buf, tempBuf, 11);
		//lcdBuffer.buf[0] = ADCReading;
		if (lcdData != NULL) {
			// Send a message to the LCD task for it to print (and the LCD task must be configured to receive this message)
			lcdBuffer.length = strlen((char*)(lcdBuffer.buf))+1;
			if (xQueueSend(lcdData->inQ,(void *) (&lcdBuffer),portMAX_DELAY) != pdTRUE) {
				VT_HANDLE_FATAL_ERROR(0);
			}
		}
		
	}
}

