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
#include "calcTask.h"
#include "i2cTemp.h"



// I have set this to a large stack size because of (a) using printf() and (b) the depth of function calls
//   for some of the calc operations
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

	const uint8_t readReg0[] = {0x00};
	const uint8_t readReg1[] = {0x01};
	const uint8_t nmeaRead[] = {0x02};
	const uint8_t nmeaRead2[] = {0x03};
	uint8_t i2c_State = 2; //set to 1 normally, 2 for m4
	uint8_t numCal[3] = { 0 }; //one entry for each pic, 1 if calibrated
	uint8_t PRead[3] = { 0 };
	uint8_t rssiString = 0;
	uint8_t nmeaString[6];
	uint8_t tempBuf[15];
	uint8_t *tempRead = tempBuf;
	uint8_t rxLen, status;
	// Get the parameters
	i2cTempStruct *param = (i2cTempStruct *) pvParameters;
	// Get the I2C device pointer
	vtI2CStruct *devPtr = param->dev;
	// Get the calc information pointer
	vtCalcStruct *calcData = param->calcData;
	vtCalcMsg calcBuffer;

	// Scale the update rate to ensure it really is in ms
	xUpdateRate = i2cREAD_RATE_BASE / portTICK_RATE_MS;

	/* We need to initialise xLastUpdateTime prior to the first call to vTaskDelayUntil(). */
	xLastUpdateTime = xTaskGetTickCount();
	
	strncpy((char *)nmeaString, (const char *) testData2, 6);
	// Like all good tasks, this should never exit
	for(;;)
	{
		/* Ask the RTOS to delay reschduling this task for the specified time */
		vTaskDelayUntil( &xLastUpdateTime, xUpdateRate );
		/*
		//PIC 0
		if (vtI2CEnQ(devPtr,0x00,0x1b,sizeof(readReg0),readReg0,1) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}

		if (vtI2CDeQ(devPtr,1,tempRead,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		
		//handle conversion of received here
		PRead[0] = tempBuf[0];
		vtITMu8(vtITMPortTempVals,rxLen); // Log the length received
		
		//PIC 1
		if (vtI2CEnQ(devPtr,0x00,0x1c,sizeof(readReg0),readReg0,1) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}

		if (vtI2CDeQ(devPtr,1,tempRead,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		
		//handle conversion of received here
		PRead[1] = tempBuf[0];
		vtITMu8(vtITMPortTempVals,rxLen); // Log the length received

		//PIC 2
		if (vtI2CEnQ(devPtr,0x00,0x1d,sizeof(readReg0),readReg0,1) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}

		if (vtI2CDeQ(devPtr,1,tempRead,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		
		//handle conversion of received here
		PRead[2] = tempBuf[0];
		vtITMu8(vtITMPortTempVals,rxLen); // Log the length received
		*/
		//Milestone 4 stuff
		
		//NMEA String read 1
		if (vtI2CEnQ(devPtr,0x00,0x1b,sizeof(readReg0),readReg0,1) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}

		if (vtI2CDeQ(devPtr,1,tempRead,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		
		vtITMu8(vtITMPortTempVals,rxLen); // Log the length received
		
		//Calculations and passing to calc
		rssiString = tempBuf[0];
				 
		//NMEA String read 1
		if (vtI2CEnQ(devPtr,0x00,0x1b,sizeof(nmeaRead),nmeaRead,6) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}

		if (vtI2CDeQ(devPtr,6,tempRead,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		
		vtITMu8(vtITMPortTempVals,rxLen); // Log the length received

		//Calculations and passing to calc
		strncpy((char *) nmeaString, (const char *) tempBuf, 6);

		if (i2c_State == 1){
			calcBuffer.buf[0] = 12;
			calcBuffer.buf[1] = 11;
			if (PRead[0] > 250 && PRead[0] > PRead[1] && PRead[0] > PRead[2] && numCal[0] == 0){
				//close to pic0
				calcBuffer.buf[2]= 0;
				strncpy((char *)calcBuffer.buf+3, (const char *) nmeaString, 6);
				if (calcData != NULL) {
					// Send a message to the calc task for it to print (and the calc task must be configured to receive this message)
					calcBuffer.length = strlen((char*)(calcBuffer.buf))+1;
					if (xQueueSend(calcData->inQ,(void *) (&calcBuffer),portMAX_DELAY) != pdTRUE) {
						VT_HANDLE_FATAL_ERROR(0);
					}
				}
				numCal[1] = 1;
			}
			else if (PRead[1] > 250 && PRead[1] > PRead[0] && PRead[1] > PRead[2] && numCal[1] == 0){
				//close to pic1
				calcBuffer.buf[2] = 1;
				strncpy((char *)calcBuffer.buf+3, (const char *) nmeaString, 6);
				if (calcData != NULL) {
					// Send a message to the calc task for it to print (and the calc task must be configured to receive this message)
					calcBuffer.length = strlen((char*)(calcBuffer.buf))+1;
					if (xQueueSend(calcData->inQ,(void *) (&calcBuffer),portMAX_DELAY) != pdTRUE) {
						VT_HANDLE_FATAL_ERROR(0);
					}
				}
				numCal[1] = 1;
			}
			else if (PRead[2] > 250 && PRead[2] > PRead[1] && PRead[2] > PRead[1] && numCal[2] == 0){
				//close to pic2
				calcBuffer.buf[2] = 2;
				strncpy((char *)calcBuffer.buf+3, (const char *) nmeaString, 6);
				if (calcData != NULL) {
					// Send a message to the calc task for it to print (and the calc task must be configured to receive this message)
					calcBuffer.length = strlen((char*)(calcBuffer.buf))+1;
					if (xQueueSend(calcData->inQ,(void *) (&calcBuffer),portMAX_DELAY) != pdTRUE) {
						VT_HANDLE_FATAL_ERROR(0);
					}
				}
				numCal[2] = 1;
			}
			if (numCal[0] == 1 && numCal[1] == 1 && numCal[2] == 1){
				i2c_State = 2;
			}
		}
		else if (i2c_State == 2){
			strncpy((char*) calcBuffer.buf, (const char*) PRead, 3);
			strncpy((char*) calcBuffer.buf + 3, (const char*) nmeaString, 6);
			if (calcData != NULL) {
				// Send a message to the calc task for it to print (and the calc task must be configured to receive this message)
				calcBuffer.length = strlen((char*)(calcBuffer.buf))+1;
				if (xQueueSend(calcData->inQ,(void *) (&calcBuffer),portMAX_DELAY) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}
			}
		}
		
	}
}

