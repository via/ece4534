#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"

/* include files. */
#include "vtUtilities.h"
#include "calcTask.h"
#include "lcdTask.h"
#include "locatelib.h"

#include "lpc17xx_gpio.h"
#define USE_GPIO 1

// I have set this to a large stack size because of (a) using printf() and (b) the depth of function calls
#if PRINTF_VERSION==1
#define calcSTACK_SIZE		8*configMINIMAL_STACK_SIZE
#else
#define calcSTACK_SIZE		4*configMINIMAL_STACK_SIZE
#endif

// Set the task up to run every 200 ms
#define taskRUN_RATE	( ( portTickType ) 1000 )

/* The LCD task. */
static portTASK_FUNCTION_PROTO( vCalcUpdateTask, pvParameters );

/*-----------------------------------------------------------*/

void vStartCalcTask( unsigned portBASE_TYPE uxPriority,vtCalcStruct *ptr )
{

	// Create the queue that will be used to talk to the LCD
	if ((ptr->inQ = xQueueCreate(vtCalcQLen,sizeof(vtCalcMsg))) == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	/* Start the task */
	portBASE_TYPE retval;
	if ((retval = xTaskCreate( vCalcUpdateTask, ( signed char * ) "Calc", calcSTACK_SIZE, (void*)ptr, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(retval);
	}
}

// This is the actual task that is run
static portTASK_FUNCTION( vCalcUpdateTask, pvParameters )
{
	portTickType xUpdateRate, xLastUpdateTime;
	vtCalcMsg msgBuffer;
	vtCalcStruct *calcPtr = (vtCalcStruct *) pvParameters;
	vtLCDStruct *lcdData = calcPtr->lcdData;
	vtFileStruct *fileData = calcPtr->fileData;
	vtFileMsg fileBuffer;
	vtLCDMsg lcdBuffer;
	
	uint8_t calcState = 1;
	uint8_t picNum = 10;
	uint8_t picCal[3] = { 0 }; //determine which pics are calibrated
	
	//Location calculation related
	double picDBW[3] = { 0.0 };
	double picDist[3] = { 0.0 };
	struct utm_coordinate * picCords[3];
	int j = 0;
	for(; j < 3; j++){
		if((picCords[j] = malloc(sizeof(struct utm_coordinate))) == NULL){
		 	// ERROR
		}
	}
	struct dms_coordinate *dmsCord;
	if((dmsCord = malloc(sizeof(struct dms_coordinate))) == NULL){
	 	// ERROR
	}
	struct utm_coordinate *utmNmea; //utm for nmea string
	if((utmNmea = malloc(sizeof(struct utm_coordinate))) == NULL){
	 	// ERROR
	}
	struct utm_coordinate *utmTx; //utm for transmitter
	if((utmTx = malloc(sizeof(struct utm_coordinate))) == NULL){
	 	// ERROR
	}
	
	const double pwr_tx = 0.0; //constant for power transmitted
	const double rc_gain = 0.0; //constant for recieve gain
	const double tx_gain = 0.0; //constant for transmit gain
	const double freq = 0.0; //const for frequency
	static const double stepSize = 1.0;

	// Scale the update rate to ensure it really is in ms
	xUpdateRate = taskRUN_RATE / portTICK_RATE_MS;

	/* We need to initialise xLastUpdateTime prior to the first call to 
	vTaskDelayUntil(). */
	xLastUpdateTime = xTaskGetTickCount();
	// Note that srand() & rand() require the use of malloc() and should not be used unless you are using
	//   MALLOC_VERSION==1
	#if MALLOC_VERSION==1
	srand((unsigned) 55); // initialize the random number generator to the same seed for repeatability
	#endif
	// Like all good tasks, this should never exit
	for(;;)
	{	
		/* Ask the RTOS to delay reschduling this task for the specified time */
		vTaskDelayUntil( &xLastUpdateTime, xUpdateRate );

		if (xQueueReceive(calcPtr->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		#if USE_GPIO == 1
		GPIO_SetValue(1, 0x80000000);
		#endif
		//Log that we are processing a message
		vtITMu8(vtITMPortLCDMsg,msgBuffer.length);

		int latDeg, lonDeg;
		float latMin, lonMin;
		if (calcState == 1){
			if (msgBuffer.buf[0] == 12 && msgBuffer.buf[1] == 11){
				picNum = msgBuffer.buf[2];
				//convert the parsed stuff to dms_coordinate struct
				if (picNum < 3){
					//sscanf((char*) msgBuffer.buf + 3, "%d %f %d %f", &latDeg, &latMin, &lonDeg, &lonMin);
					dmsCord->latDegrees = (int) msgBuffer.latDeg;
					dmsCord->latMinutes = (double) msgBuffer.latMin;
					dmsCord->lonDegrees = (int) msgBuffer.lonDeg;
					dmsCord->lonMinutes = (double) msgBuffer.lonMin;
					convertDMS_to_UTM( dmsCord, picCords[picNum] );
					picCal[picNum] = 1;
				}
				
			}
			if (msgBuffer.buf[0] == 0xD0 && msgBuffer.buf[1] == 0xCF && msgBuffer.buf[2] == 0x11){
				lcdBuffer.buf[0] = 0xDE;
				lcdBuffer.buf[1] = 0xAD;
				lcdBuffer.buf[2] = 0xBE;
					if (lcdData != NULL) {
						lcdBuffer.length = strlen((char*)(lcdBuffer.buf))+1;
						if (xQueueSend(lcdData->inQ,(void *) (&lcdBuffer),portMAX_DELAY) != pdTRUE) {
							VT_HANDLE_FATAL_ERROR(0);
						}
					}
				calcState = 2;
			 }

		}
		else if (calcState == 2){
			//convert PIC rssi to dBW
			
			picDBW[0] = convert_rssi_to_db( msgBuffer.buf[0] );
			picDBW[1] = convert_rssi_to_db( msgBuffer.buf[1] ); //+1 when not M4
			picDBW[2] = convert_rssi_to_db( msgBuffer.buf[2] ); //+2 when not M4
			
			//Take the nmea data and put it into dmsCord here
			//sscanf((char*) (msgBuffer.buf+3), "%d %f %d %f", &latDeg, &latMin, &lonDeg, &lonMin);
			dmsCord->latDegrees = (int) msgBuffer.latDeg;
			dmsCord->latMinutes = (double) msgBuffer.latMin;

			dmsCord->lonDegrees = (int) msgBuffer.lonDeg;
			dmsCord->lonMinutes = (double) msgBuffer.lonMin;

			//Convert to UTM to do ?? with it
			convertDMS_to_UTM( dmsCord, utmNmea );
			
			//Get distances to each transmitter
			picDist[0] = distance_to_transmitter( picDBW[0], pwr_tx, rc_gain, tx_gain, freq );
			picDist[1] = distance_to_transmitter( picDBW[1], pwr_tx, rc_gain, tx_gain, freq );
			picDist[2] = distance_to_transmitter( picDBW[2], pwr_tx, rc_gain, tx_gain, freq );
			
			//Calculate estimated position, run function 16 times
			uint8_t count;
			for (count = 0; count < 16; count++){
				location_gradient_descent( picCords, picDist, utmTx, stepSize ); 
				}
			// TODO: change later 05/01/2012 1428	
			sprintf((char*)(lcdBuffer.buf),"E: %2.2f N: %2.2f", utmTx->eastings, utmTx->northings);
			if (lcdData != NULL) {
				lcdBuffer.length = strlen((char*)(lcdBuffer.buf))+1;
				if (xQueueSend(lcdData->inQ,(void *) (&lcdBuffer),portMAX_DELAY) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}
			}

			fileBuffer.buf[0] = '\xCA';
			fileBuffer.buf[1] = '\xFE';
			sprintf((char*)(fileBuffer.buf + 2), "%7.3f %7.3f %7.3f %7.3f",
					utmTx->eastings, utmTx->northings, utmNmea->eastings, utmNmea->northings);
			if (fileData != NULL) {
				fileBuffer.length = strlen((char*)(fileBuffer.buf))+1;
				if (xQueueSend(lcdData->inQ,(void *) (&fileBuffer),portMAX_DELAY) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}
			}
		}  
		#if USE_GPIO == 1
		GPIO_ClearValue(1, 0x80000000);
		#endif
	}
}

