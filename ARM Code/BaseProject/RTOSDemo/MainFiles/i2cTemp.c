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
#include "lpc17xx_gpio.h"

// I have set this to a large stack size because of (a) using printf() and (b) the depth of function calls
//   for some of the calc operations
#if PRINTF_VERSION==1
#define i2cSTACK_SIZE		16*configMINIMAL_STACK_SIZE
#else
#define i2cSTACK_SIZE		8*configMINIMAL_STACK_SIZE
#endif

// Set the task up to run every second (need to modify this to poll more often from the adc)
#define i2cREAD_RATE_BASE	( ( portTickType ) 1000)

#define USE_GPIO 1

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
	const uint8_t TSC_FIFO[] = {0x4C};
	const uint8_t tscRead[] = {0xD7};
	
	/*
	1 - Touchscreen reading
	2 - Node calibration
	3 - Normal operation
	*/
	uint8_t i2c_State = 1; 

	//Rolling gps avg vars
	int latDeg = 0;
	double latMin = 0.0;
	int lonDeg = 0;
	double lonMin = 0.0;
	uint8_t avgCount = 0;
	double tempMin = 0.0;
	
	//gps values
	int glatDeg = 0;
	double glatMin = 0.0;
	int glonDeg = 0;
	double glonMin = 0.0;
	
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
	//LCD info
	vtLCDStruct *lcdData = param->lcdData;
	vtLCDMsg lcdBuffer;

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
	
	//Set up initial LCD display
	sprintf((char*)(lcdBuffer.buf),"Tap to calibrate #0");
					if (lcdData != NULL) {
						// Send a message to the calc task for it to print (and the calc task must be configured to receive this message)
						lcdBuffer.length = strlen((char*)(lcdBuffer.buf))+1;
						if (xQueueSend(lcdData->inQ,(void *) (&lcdBuffer),portMAX_DELAY) != pdTRUE) {
							VT_HANDLE_FATAL_ERROR(0);
						}
					}
	// Like all good tasks, this should never exit
	for(;;)
	{
		/* Ask the RTOS to delay reschduling this task for the specified time */
		vTaskDelayUntil( &xLastUpdateTime, xUpdateRate );
		
		#if USE_GPIO == 1
		GPIO_SetValue(0,0x00040000); //pin 18
		#endif
		
		//Do requests from PIC0 and processing of data here
		//nmeaString will be formatted later, for now sprintf the nmea into 
		//glatDeg, glatMin, glonDeg, glonMin
		glatDeg = 0;
		glatMin = 0.0;
		glonDeg = 0;
		glonMin = 0.0;
		 
		
		
		//Begin i2c state machine
		if (i2c_State == 1){
			//i2c read from lcd to see if interrupt triggered
			if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INT),TSC_INT,1) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}

			if (vtI2CDeQ(tscPtr,1,tempRead,&rxLen,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
			}

			if (numCal >= 3) {
					i2c_State = 3;
					calcBuffer.buf[0] = 0xD0;
					calcBuffer.buf[1] = 0xCF;
					calcBuffer.buf[2] = 0x11;
					if (calcData != NULL) {
						// Send a message to the calc task for it to print (and the calc task must be configured to receive this message)
						calcBuffer.length = strlen((char*)(calcBuffer.buf))+1;
						if (xQueueSend(calcData->inQ,(void *) (&calcBuffer),portMAX_DELAY) != pdTRUE) {
							VT_HANDLE_FATAL_ERROR(0);
						}
					}
				}

			else if ((tempBuf[0] & 0x02) | (tempBuf[0] & 0x01)) {
				//Read X,Y values, need to recombine them later
				if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(tscRead),tscRead,1) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}

				if (vtI2CDeQ(tscPtr,1,tempRead,&rxLen,&status) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}
				
				if (tempBuf[0] > 30){
					i2c_State = 2;
					latDeg = 0;
					latMin = 0.0;
					lonDeg = 0;
					lonMin = 0.0;
					avgCount = 0;
					sprintf((char*)(lcdBuffer.buf),"Calibrating Node #%d", numCal);
					if (lcdData != NULL) {
						// Send a message to the calc task for it to print (and the calc task must be configured to receive this message)
						lcdBuffer.length = strlen((char*)(lcdBuffer.buf))+1;
						if (xQueueSend(lcdData->inQ,(void *) (&lcdBuffer),portMAX_DELAY) != pdTRUE) {
							VT_HANDLE_FATAL_ERROR(0);
						}
					}
				}

				//clear LCD interrupt
				if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INIT_13),TSC_INIT_13,0) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}
				if (vtI2CDeQ(tscPtr,0,NULL,&rxLen,&status) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}
			}
		}
		else if (i2c_State == 2){
			if (avgCount < 10){
				//Cumulative moving average
				latDeg = ((latDeg * avgCount) + glatDeg) / (avgCount + 1);
				latMin = ((latMin * avgCount) + glatMin) / (avgCount + 1);
				lonDeg = ((lonDeg * avgCount) + glonDeg) / (avgCount + 1);
				lonMin = ((lonMin * avgCount) + glonMin) / (avgCount + 1);
				
				avgCount = avgCount + 1;
			}
			else {
				//Need to convert GPS to nmeaString format here
				uint16_t tMin = 0;
				
				nmeaString[0] = (uint8_t) latDeg;
				latMin = latMin * 1000;
				tMin = (uint16_t) latMin;
				nmeaString[1] = (uint8_t) ((tMin & 0xFF00) >> 8);
				nmeaString[2] = (uint8_t) (tMin & 0x00FF);
				
				nmeaString[3] = (uint8_t) lonDeg;
				lonMin = lonMin * 1000;
				tMin = (uint16_t) lonMin;
				nmeaString[4] = (uint8_t) ((tMin & 0xFF00) >> 8);
				nmeaString[5] = (uint8_t) (tMin & 0x00FF);
				
				calcBuffer.buf[0] = 12;
				calcBuffer.buf[1] = 11;
				calcBuffer.buf[2] = numCal;
					//screen triggered, do passing of things to calc
					strncpy((char *)calcBuffer.buf+3, (const char *) nmeaString, 6);
					if (calcData != NULL) {
						// Send a message to the calc task for it to print (and the calc task must be configured to receive this message)
						calcBuffer.length = strlen((char*)(calcBuffer.buf))+1;
						if (xQueueSend(calcData->inQ,(void *) (&calcBuffer),portMAX_DELAY) != pdTRUE) {
							VT_HANDLE_FATAL_ERROR(0);
						}
					}
					
					if (numCal < 2) {
						//Show that we are done calibrating
						sprintf((char*)(lcdBuffer.buf),"Tap to calibrate #%d", numCal+1);
						if (lcdData != NULL) {
							// Send a message to the calc task for it to print (and the calc task must be configured to receive this message)
							lcdBuffer.length = strlen((char*)(lcdBuffer.buf))+1;
							if (xQueueSend(lcdData->inQ,(void *) (&lcdBuffer),portMAX_DELAY) != pdTRUE) {
								VT_HANDLE_FATAL_ERROR(0);
							}
						}
						//Clear any touchscreen interrupts that may have come up
						if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(tscRead),tscRead,1) != pdTRUE) {
							VT_HANDLE_FATAL_ERROR(0);
						}

						if (vtI2CDeQ(tscPtr,1,tempRead,&rxLen,&status) != pdTRUE) {
							VT_HANDLE_FATAL_ERROR(0);
						}
						
						if (vtI2CEnQ(tscPtr,0x01,0x41,sizeof(TSC_INIT_13),TSC_INIT_13,0) != pdTRUE) {
							VT_HANDLE_FATAL_ERROR(0);
						}
						if (vtI2CDeQ(tscPtr,0,NULL,&rxLen,&status) != pdTRUE) {
							VT_HANDLE_FATAL_ERROR(0);
						}
					}
					
					i2c_State = 1;
					numCal = numCal + 1;
				}
		}
		else if (i2c_State == 3){
			//Need to convert GPS to nmeaString format here
			uint16_t tMin = 0;
				
			nmeaString[0] = (uint8_t) glatDeg;
			glatMin = glatMin * 1000;
			tMin = (uint16_t) glatMin;
			nmeaString[1] = (uint8_t) ((tMin & 0xFF00) >> 8);
			nmeaString[2] = (uint8_t) (tMin & 0x00FF);
				
			nmeaString[3] = (uint8_t) glonDeg;
			glonMin = glonMin * 1000;
			tMin = (uint16_t) glonMin;
			nmeaString[4] = (uint8_t) ((tMin & 0xFF00) >> 8);
			nmeaString[5] = (uint8_t) (tMin & 0x00FF);
			
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
		#if USE_GPIO == 1
		GPIO_ClearValue(0,0x00040000);
		#endif
	}
}
