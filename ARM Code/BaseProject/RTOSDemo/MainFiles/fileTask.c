#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* include files. */
#include "vtUtilities.h"
#include "fileTask.h"
#include "lcdtask.h"
#include "diskio.h"
#include "ff.h"
#include "webdata.h"
#include "lpc17xx_gpio.h"

#define MILESTONE_FILE 1
#define USE_GPIO 0

// I have set this to a large stack size because of (a) using ////printf() and (b) the depth of function calls
#if printf_VERSION==1
#define fileSTACK_SIZE		8*configMINIMAL_STACK_SIZE
#else
#define fileSTACK_SIZE		4*configMINIMAL_STACK_SIZE
#endif

// Set the task up to run every 200 ms
#define taskRUN_RATE	( ( portTickType ) 1000 )

static portTASK_FUNCTION_PROTO( vFileTask, pvParameters );
static FRESULT F_Write(BYTE Msg[], UINT msg_size, TCHAR *path, int append);
static FRESULT F_Read (BYTE Buf[], UINT buf_size, TCHAR *path);

/*-----------------------------------------------------------*/

void vStartFileTask( unsigned portBASE_TYPE uxPriority,vtFileStruct *ptr )
{

	// Create the queue that will be used to talk to the LCD
	if ((ptr->inQ = xQueueCreate(vtFileQLen,sizeof(vtFileMsg))) == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	/* Start the task */
	portBASE_TYPE retval;
	if ((retval = xTaskCreate( vFileTask, ( signed char * ) "File", fileSTACK_SIZE, (void*)ptr, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(retval);
	}
}

static FATFS fatfs;  // Filesystem object
static FIL fil;   	 // File object
static uint8_t data_buf[256];
static uint8_t temp_buf[256];

static portTASK_FUNCTION( vFileTask, pvParameters )
{
	xSemaphore = xSemaphoreCreateMutex();
	
	portTickType xUpdateRate, xLastUpdateTime;
	vtFileMsg msgBuffer;
	vtFileStruct *filePtr = (vtFileStruct *) pvParameters;
	//vtLCDStruct *lcdData = filePtr->lcdData;
	//vtLCDMsg lcdBuffer;
	
    TCHAR *FilePath = "0:data.txt" ; // file path
	
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
	
	// === MOUNT ===
	f_mount(0, &fatfs);  /* Register volume work area (never fails) */
	
	// === UMOUNT ===
    //f_mount(0, NULL);/* Unregister work area prior to discard it */
	
    //------------------------------------
    // Like all good tasks, this should never exit
	
	#if MILESTONE_FILE == 1
	// Temp, remove later on
	msgBuffer.buf[0] = '\xCA';
	msgBuffer.buf[1] = '\xFE';
	sprintf((char*)msgBuffer.buf + 2, "1111111 2222222 3333333 4444444");
	msgBuffer.length = strlen((char*)msgBuffer.buf);
	if(xQueueSend(filePtr->inQ,(void*) (&msgBuffer),portMAX_DELAY) != pdTRUE){
		VT_HANDLE_FATAL_ERROR(0);
	}
	int j = 1;
	#endif
	for(;;)
	{	
		/* Ask the RTOS to delay reschduling this task for the specified time */
		vTaskDelayUntil( &xLastUpdateTime, xUpdateRate );

		if (xQueueReceive(filePtr->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		
		//Log that we are processing a message
		vtITMu8(vtITMPortLCDMsg,msgBuffer.length);
		
		#if USE_GPIO == 1
		GPIO_SetValue(1, 0x10000000);
		#endif
		
		if(msgBuffer.buf[0] & 0xCA && msgBuffer.buf[1] & 0xFE){
			float e_calc, n_calc, e_actual, n_actual;
			// sscanf the values needed
			sscanf((char*)msgBuffer.buf + 2, "%f %f %f %f", 
					&e_calc, &n_calc, &e_actual, &n_actual);
			
			// sprintf to buffer to line formatted as needed
			BYTE line[64];
			sprintf((char*)line, "%7.3f\t%7.3f\t%7.3f\t%7.3f\n",
					e_calc, n_calc, e_actual, n_actual);
			
			// read here for insurance in other lines if needed, not used
			//BYTE buf[64];
			//rc = F_Read(buf, sizeof(buf), FilePath);
			if(xSemaphore != NULL){
				if(xSemaphoreTake( xSemaphore, (portTickType) 20) == pdTRUE ){
					// write new line to file
					FRESULT rc;			
					rc = F_Write((BYTE*)line, strlen((char*)line), FilePath, 1);
					// Clear buffer when almost full
					if( (strlen((char*)data_buf) + strlen((char*)line) + 1) > sizeof(data_buf) ){
						memset(data_buf, 0, sizeof(data_buf));
					}
					// Or roll out one old line to make room for new line
					/*while( (strlen((char*)data_buf) + strlen((char*)line) + 1) > sizeof(data_buf) ){
						char* pos = strchr((char*)data_buf, '\n');
						int offset = (int)(pos - (char*)data_buf);
						memset(temp_buf, 0, sizeof(temp_buf));
						memcpy(temp_buf, data_buf, strlen((char*)data_buf));
						memset(data_buf, 0, sizeof(data_buf));
						memcpy(data_buf, temp_buf + offset + 1, strlen((char*)temp_buf) - offset - 1);
					}*/
					// add new line to buffer
					strncat((char*)data_buf, (char*)line, strlen((char*)line));
					xSemaphoreGive( xSemaphore );
				}
			}
			else{
				// take semaphore failed
			}
		}
		else{
			// Do nothing
		}
		#if USE_GPIO == 1
		GPIO_ClearValue(1, 0x10000000);
		#endif
		#if MILESTONE_FILE == 1
		if(j < 100){
			msgBuffer.buf[0] = '\xCA';
			msgBuffer.buf[1] = '\xFE';
			sprintf((char*)msgBuffer.buf + 2, "1111111 2222222 3333333 4444444");		
			j++;
			msgBuffer.length = strlen((char*)msgBuffer.buf);
			if(xQueueSend(filePtr->inQ,(void*) (&msgBuffer),portMAX_DELAY) != pdTRUE){
				VT_HANDLE_FATAL_ERROR(0);
			}
		}
		#endif
	}
    //vTaskDelete(NULL);
} 

FRESULT F_Write(BYTE Message[], UINT msg_size, TCHAR *filepath, int append){
	FRESULT rc;
	UINT bw = 0;
	
	// === Write test ===
    rc = f_open(&fil, filepath, FA_WRITE | FA_OPEN_ALWAYS);
    if(rc){
		//printf("File open error = %u",rc);
		return rc;
    }
	
	// Seek end if append
	if(append){
		rc = f_lseek(&fil, f_size(&fil));
		if(rc){
			f_close(&fil);
			return rc;
		}
	}

    rc = f_write(&fil, Message, msg_size, &bw); // write file
    if(rc){
		//printf("Write error = %u", rc);
		f_close(&fil);
		return rc;
    }

    rc = f_close(&fil);
    if(rc){
		//printf("File close error = %u", rc);
		return rc;
    }
	
	return rc;
}

FRESULT F_Read(BYTE Buf[], UINT buf_size, TCHAR *filepath){
	FRESULT rc;
	UINT numread = 0;
	UINT br = 0;
	
	// === Read test ===
    rc = f_open(&fil, filepath, FA_READ | FA_OPEN_EXISTING);
    if(rc){
		//printf("File open error = %u", rc);
    }

    for(;;){
		rc = f_read(&fil, Buf, buf_size, &br);	// Read chunk of file
		if(rc || !br){
			break;         							// Error or EOF
		}
		int i = 0;
		for(; i < br; i++){
			putchar(Buf[i]);
		}
		numread += br;
    }
    if(rc){
		//printf("Read error = %u", rc);
	}

    rc = f_close(&fil);
    if(rc){
		//printf("File close error = %u", rc);
    }	
	
	return rc;
}

void prep_data(void *buf){
	strncpy((char*)buf, (char*) data_buf, sizeof(data_buf));
}

















