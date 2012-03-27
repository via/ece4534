#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"

/* include files. */
#include "vtUtilities.h"
#include "calcTask.h"
#include "lcdTask.h"
#include "locatelib.h"

// I have set this to a large stack size because of (a) using printf() and (b) the depth of function calls
#if PRINTF_VERSION==1
#define calcSTACK_SIZE		8*configMINIMAL_STACK_SIZE
#else
#define calcSTACK_SIZE		4*configMINIMAL_STACK_SIZE
#endif

// Set the task up to run every 200 ms
#define taskRUN_RATE	( ( portTickType ) 200 )

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
	vtLCDMsg lcdBuffer;
	
	calcState = 1;
	uint8_t picCal[2] = { 0 }; //determine which pics are calibrated
	
	//Location calculation related
	double picDBW[2] = { 0.0 };
	double picDist[2] = { 0.0 };
	utm_coordinate picCords[2];
	dms_coordinate dmsCord;
	utm_coordinate utmNmea; //utm for nmea string
	utm_coordinate utmTx; //utm for transmitter
	
	const double pwr_tx = 0.0; //constant for power transmitted
	const double rc_gain = 0.0; //constant for recieve gain
	const double tx_gain = 0.0; //constant for transmit gain
	const double freq = 0.0; //const for frequency
	const double stepSize = 1.0;

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
		//Log that we are processing a message
		vtITMu8(vtITMPortLCDMsg,msgBuffer.length);
		if (calcState == 1){
			if (msgBuffer.buf[0] == 12 && msgBuffer.buf[1] == 11){
				//convert the parsed stuff to dms_coordinate struct
				dmsCord.latDegrees = (int) msgBuffer.buf[3];
				dmsCord.latMinutes = (double) msgBuffer.buf[4];
				dmsCord.latMinutes = dmsCord.latMinutes << 4;
				dmsCord.latMinutes = dmsCord.latMinutes + (double) msgBuffer.buf[5];
				dmsCord.lonDegrees = (int) msgBuffer.buf[6];
				dmsCord.lonMinutes = (double) msgBuffer.buf[7];
				dmsCord.lonMinutes = dmsCord.latMinutes << 4;
				dmsCord.lonMinutes = dmsCord.latMinutes + (double) msgBuffer.buf[8];
				convertDMS_to_UTM( dmsCord, picCords[msgBuffer.buf[2]] );
				picCal[msgBuffer.buf[2]] = 1;
			}
			if (picCal[0] == 1 && picCal[1] == 1 && picCal[2] == 1)
				calcState = 2;
			
			sprintf((char*)(lcdBuffer.buf),"Calibrating");
			if (lcdData != NULL) {
				lcdBuffer.length = strlen((char*)(lcdBuffer.buf))+1;
				if (xQueueSend(lcdData->inQ,(void *) (&lcdBuffer),portMAX_DELAY) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}
			}
		}
		else if (calcState == 2){
			//convert PIC rssi to dBW
			picDBW[0] = convert_rssi_to_db( msgBuffer.buf[0] );
			picDBW[1] = convert_rssi_to_db( msgBuffer.buf[2] );
			picDBW[2] = convert_rssi_to_db( msgBuffer.buf[4] );
			
			//Take the nmea data and put it into dmsCord here
			dmsCord.latDegrees = (int) msgBuffer.buf[3];
			dmsCord.latMinutes = (double) msgBuffer.buf[4];
			dmsCord.latMinutes = dmsCord.latMinutes << 4;
			dmsCord.latMinutes = dmsCord.latMinutes + (double) msgBuffer.buf[5];
			dmsCord.lonDegrees = (int) msgBuffer.buf[6];
			dmsCord.lonMinutes = (double) msgBuffer.buf[7];
			dmsCord.lonMinutes = dmsCord.latMinutes << 4;
			dmsCord.lonMinutes = dmsCord.latMinutes + (double) msgBuffer.buf[8];
			
			//Convert to UTM to do ?? with it
			convertDMS_to_UTM( dmsCord, utmNmea );
			
			//Get distances to each transmitter
			picDist[0] = distance_to_transmitter( picDBW[0], pwr_tx, rc_gain, tx_gain, freq );
			picDist[1] = distance_to_transmitter( picDBW[1], pwr_tx, rc_gain, tx_gain, freq );
			picDist[2] = distance_to_transmitter( picDBW[2], pwr_tx, rc_gain, tx_gain, freq );
			
			//Calculate estimated position
			location_gradient_descent( picCords, picDist, utmTx, stepSize ); 
			sprintf((char*)(lcdBuffer.buf),"E: %2.2f N: %2.2f", utmTx.eastings, utmTx.northings);
			//Do Stuff here, msgBuffer.buf for message contents
			if (lcdData != NULL) {
				lcdBuffer.length = strlen((char*)(lcdBuffer.buf))+1;
				if (xQueueSend(lcdData->inQ,(void *) (&lcdBuffer),portMAX_DELAY) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}
			}
		}
	}
}

