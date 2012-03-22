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
		
		sprintf((char*)(lcdBuffer.buf),"Current Number is: %u", (int)msgBuffer[0]);
		//Do Stuff here, msgBuffer.buf for message contents
		
		lcdBuffer.length = strlen((char*)(lcdBuffer.buf))+1;
			if (xQueueSend(lcdData->inQ,(void *) (&lcdBuffer),portMAX_DELAY) != pdTRUE) {
				VT_HANDLE_FATAL_ERROR(0);
			}
	}
}

