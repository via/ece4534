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
#define i2cSTACK_SIZE		16*configMINIMAL_STACK_SIZE
#else
#define i2cSTACK_SIZE		8*configMINIMAL_STACK_SIZE
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
	//Commands for initializing touchscreen
	const uint8_t TSC_INIT_1[] = {0x40, 0x09}; //z only
	const uint8_t TSC_INIT_2[] = {0x03, 0x02}; //reset controller
	const uint8_t TSC_INIT_3[] = {0x04, 0x0C}; //clock
	const uint8_t TSC_INIT_4[] = {0x0A, 0x07}; //int enable
	const uint8_t TSC_INIT_5[] = {0x20, 0x69}; //adc
	const uint8_t TSC_INIT_6[] = {0x21, 0x01}; //adc
	const uint8_t TSC_INIT_7[] = {0x41, 0xF5}; //tsc config
	const uint8_t TSC_INIT_8[] = {0x4A, 0x01}; //single pt reader
	const uint8_t TSC_INIT_9[] = {0x4B, 0x01, 0x00}; //fifo status
	const uint8_t TSC_INIT_10[] = {0x56, 0x07}; //fraction z cfg
	const uint8_t TSC_INIT_11[] = {0x58, 0x01}; //set tsc I drive
	const uint8_t TSC_INIT_12[] = {0x40, 0x09}; //z only
	const uint8_t TSC_INIT_13[] = {0x0B, 0xFF}; //clear interrupts reuse later
	const uint8_t TSC_INIT_14[] = {0x09, 0x01}; //enable interrupts
	
	const uint8_t TSC_INT[] = {0x0B}; //normally 0B
	const uint8_t zRead[] = {0xD7};
	
	uint8_t i2c_State = 1; //1 to start at calibration, 2 for skipping
	uint8_t numCal = 0;
	uint8_t PRead[3] = { 0 };
	uint8_t rssiString = 0;
	uint8_t nmeaString[6] = { 0 };
	uint8_t tempBuf[15];
	uint8_t *tempRead = tempBuf;
	uint8_t rxLen, status;
	// Get the parameters
	i2cTempStruct *param = (i2cTempStruct *) pvParameters;
	// Get the I2C device pointer
	vtI2CStruct *devPtr = param->dev0;
	vtI2CStruct *tscPtr = param->dev1;
	// Get the calc information pointer
	vtCalcStruct *calcData = param->calcData;
	vtCalcMsg calcBuffer;

	// Scale the update rate to ensure it really is in ms
	xUpdateRate = i2cREAD_RATE_BASE / portTICK_RATE_MS;

	/* We need to initialise xLastUpdateTime prior to the first call to vTaskDelayUntil(). */
	xLastUpdateTime = xTaskGetTickCount();
	
	//Begin touchscreen initialization
	if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INIT_1),TSC_INIT_1,0) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
	if (vtI2CDeQ(tscPtr,0,NULL,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}

	if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INIT_2),TSC_INIT_2,0) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
	if (vtI2CDeQ(tscPtr,0,NULL,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
	
	if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INIT_3),TSC_INIT_3,0) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
	if (vtI2CDeQ(tscPtr,0,NULL,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}

	vTaskDelay(10/ portTICK_RATE_MS );
	
	if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INIT_4),TSC_INIT_4,0) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
	if (vtI2CDeQ(tscPtr,0,NULL,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
			
	if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INIT_5),TSC_INIT_5,0) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
	if (vtI2CDeQ(tscPtr,0,NULL,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
			
	vTaskDelay(2/ portTICK_RATE_MS );
			
	if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INIT_6),TSC_INIT_6,0) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
	if (vtI2CDeQ(tscPtr,0,NULL,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
			
	if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INIT_7),TSC_INIT_7,0) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
	if (vtI2CDeQ(tscPtr,0,NULL,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
			
	if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INIT_8),TSC_INIT_8,0) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
	if (vtI2CDeQ(tscPtr,0,NULL,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
			
	if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INIT_9),TSC_INIT_9,0) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
	if (vtI2CDeQ(tscPtr,0,NULL,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}

	if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INIT_10),TSC_INIT_10,0) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
	if (vtI2CDeQ(tscPtr,0,NULL,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
			
	if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INIT_11),TSC_INIT_11,0) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
	if (vtI2CDeQ(tscPtr,0,NULL,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
			
	if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INIT_12),TSC_INIT_12,0) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
	if (vtI2CDeQ(tscPtr,0,NULL,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}

	//Stops working sometime after this, both commands cause issues
		
	if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INIT_13),TSC_INIT_13,0) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
	if (vtI2CDeQ(tscPtr,0,NULL,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
	
	if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INIT_14),TSC_INIT_14,0) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}
	if (vtI2CDeQ(tscPtr,0,NULL,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}

	// Like all good tasks, this should never exit
	for(;;)
	{
		/* Ask the RTOS to delay reschduling this task for the specified time */
		vTaskDelayUntil( &xLastUpdateTime, xUpdateRate );

		//Do requests from PIC0 and processing of data here
		 
		
		
		//Begin i2c state machine
		if (i2c_State == 1){
			calcBuffer.buf[0] = 12;
			calcBuffer.buf[1] = 11;
			tempBuf[0] = 5;
			//i2c read from lcd to see if interrupt triggered
			if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INT),TSC_INT,1) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}

			if (vtI2CDeQ(tscPtr,1,tempRead,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}

			if ((tempBuf[0] & 0x02) | (tempBuf[0] & 0x01)) {
				calcBuffer.buf[2] = numCal;
				
				if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(zRead),zRead,1) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}

				if (vtI2CDeQ(tscPtr,1,tempRead,&rxLen,&status) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}
				
				//screen triggered, do passing of things to calc
				strncpy((char *)calcBuffer.buf+3, (const char *) nmeaString, 6);
				if (calcData != NULL) {
					// Send a message to the calc task for it to print (and the calc task must be configured to receive this message)
					calcBuffer.length = strlen((char*)(calcBuffer.buf))+1;
					if (xQueueSend(calcData->inQ,(void *) (&calcBuffer),portMAX_DELAY) != pdTRUE) {
						VT_HANDLE_FATAL_ERROR(0);
					}
				}
				//clear LCD interrupt
				if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INIT_13),TSC_INIT_13,0) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}
				if (vtI2CDeQ(tscPtr,0,NULL,&rxLen,&status) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}
				numCal = numCal + 1;
			}
			if (numCal == 2){
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

