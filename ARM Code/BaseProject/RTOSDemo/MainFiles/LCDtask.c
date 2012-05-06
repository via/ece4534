#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"

/* include files. */
#include "GLCD.h"
#include "vtUtilities.h"
#include "LCDtask.h"
#include "vtI2C.h"

#include "lpc17xx_gpio.h"
#define USE_GPIO 0

// I have set this to a large stack size because of (a) using printf() and (b) the depth of function calls
//   for some of the LCD operations
#if PRINTF_VERSION==1
#define lcdSTACK_SIZE		8*configMINIMAL_STACK_SIZE
#else
#define lcdSTACK_SIZE		4*configMINIMAL_STACK_SIZE
#endif

// Set the task up to run every 200 ms
#define lcdWRITE_RATE_BASE	( ( portTickType ) 1000 )

/* The LCD task. */
static portTASK_FUNCTION_PROTO( vLCDUpdateTask, pvParameters );

/*-----------------------------------------------------------*/

void vStartLCDTask( unsigned portBASE_TYPE uxPriority,vtLCDStruct *ptr )
{

	// Create the queue that will be used to talk to the LCD
	if ((ptr->inQ = xQueueCreate(vtLCDQLen,sizeof(vtLCDMsg))) == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	/* Start the task */
	portBASE_TYPE retval;
	if ((retval = xTaskCreate( vLCDUpdateTask, ( signed char * ) "LCD", lcdSTACK_SIZE, (void*)ptr, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(retval);
	}
}

int LCD_STATE = 2;

#include "bgclb.c"


// Convert from HSL colormap to RGB values in this weird colormap
// H: 0 to 360
// S: 0 to 1
// L: 0 to 1
// The LCD has a funky bitmap.  Each pixel is 16 bits (a "short unsigned int")
//   Red is the most significant 5 bits
//   Blue is the least significant 5 bits
//   Green is the middle 6 bits
static unsigned short hsl2rgb(float H,float S,float L)
{
	float C = (1.0 - fabs(2.0*L-1.0))*S;
	float Hprime = H / 60;
	unsigned short t = Hprime / 2.0;
	t *= 2;
	float X = C * (1-abs((Hprime - t) - 1));
	unsigned short truncHprime = Hprime;
	float R1, G1, B1;

	switch(truncHprime) {
		case 0: {
			R1 = C; G1 = X; B1 = 0;
			break;
		}
		case 1: {
			R1 = X; G1 = C; B1 = 0;
			break;
		}
		case 2: {
			R1 = 0; G1 = C; B1 = X;
			break;
		}
		case 3: {
			R1 = 0; G1 = X; B1 = C;
			break;
		}
		case 4: {
			R1 = X; G1 = 0; B1 = C;
			break;
		}
		case 5: {
			R1 = C; G1 = 0; B1 = X;
			break;
		}
		default: {
			// make the compiler stop generating warnings
			R1 = 0; G1 = 0; B1 = 0;
			VT_HANDLE_FATAL_ERROR(Hprime);
			break;
		}
	}
	float m = L - 0.5*C;
	R1 += m; G1 += m; B1 += m;
	unsigned short red = R1*32; if (red > 31) red = 31;
	unsigned short green = G1*64; if (green > 63) green = 63;
	unsigned short blue = B1*32; if (blue > 31) blue = 31;
	unsigned short color = (red << 11) | (green << 5) | blue;
	return(color); 
}

// This is the actual task that is run
static portTASK_FUNCTION( vLCDUpdateTask, pvParameters )
{
	portTickType xUpdateRate, xLastUpdateTime;
	vtLCDMsg msgBuffer;
	vtLCDStruct *lcdPtr = (vtLCDStruct *) pvParameters;
	//counter for line display
	//uint8_t counter = 0;
	/* Initialize the LCD */
	GLCD_Init();
	GLCD_Clear(White);

	const char line_0[] = "Calc. Position";
	const char line_2[] = "Bearing from #0";
	const char line_4[] = "GPS";
	const char line_6[] = "Bearing from GPS";
	const char line_8[] = "RSSI [0, 1, 2]";
	int initial = 0;

	// Scale the update rate to ensure it really is in ms
	xUpdateRate = lcdWRITE_RATE_BASE / portTICK_RATE_MS;

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
	if (LCD_STATE == 2){
		/* Ask the RTOS to delay reschduling this task for the specified time */
		vTaskDelayUntil( &xLastUpdateTime, xUpdateRate );
		// wait for a message from another task telling us to send/recv over i2c
		
		if (xQueueReceive(lcdPtr->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}

		//Log that we are processing a message
		vtITMu8(vtITMPortLCDMsg,msgBuffer.length);

		if (msgBuffer.buf[0] == 0xDE && msgBuffer.buf[1] == 0xAD && msgBuffer.buf[2] == 0xBE){
			GLCD_Clear(White);
			LCD_STATE = 3;		
		}
		else {
			char buf[21];
			sprintf(buf, "%4.6f, %4.6f", msgBuffer.tlat, msgBuffer.tlon);
			GLCD_DisplayString(0,0,1,(unsigned char *)msgBuffer.buf);
			GLCD_DisplayString(1,0,1,(unsigned char *) buf);
		}
	}
	else if (LCD_STATE == 3){
		/* Ask the RTOS to delay reschduling this task for the specified time */
		//vTaskDelayUntil( &xLastUpdateTime, xUpdateRate );

		// wait for a message from another task telling us to send/recv over i2c
		if (xQueueReceive(lcdPtr->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		#if USE_GPIO == 1
		GPIO_SetValue(1, 0x20000000);
		#endif
		//Log that we are processing a message
		vtITMu8(vtITMPortLCDMsg,msgBuffer.length);
		// Decide what color and then clear the line
		GLCD_SetTextColor(Black);
		GLCD_SetBackColor(White);

		if(!initial){
			GLCD_DisplayString(0, 0, 1, (unsigned char *)line_0);
			GLCD_DisplayString(2, 0, 1, (unsigned char *)line_2);
			GLCD_DisplayString(4, 0, 1, (unsigned char *)line_4);
			GLCD_DisplayString(6, 0, 1, (unsigned char *)line_6);
			GLCD_DisplayString(8, 0, 1, (unsigned char *)line_8);
			initial++;
		}
		GLCD_DisplayString(msgBuffer.line_num + 1, 0, 1, (unsigned char *)msgBuffer.buf);
		#if USE_GPIO == 1
		GPIO_ClearValue(1, 0x20000000);
		#endif
	}
else{
	//	Bad setting
	}	
	}
}