#include <stdlib.h>
#include <stdio.h>

#include "vtI2C.h"
/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "semphr.h"

/* include files. */
#include "lpc17xx_i2c.h"
#include "vtUtilities.h"

#include "lpc17xx_libcfg_default.h"
#include "lpc17xx_pinsel.h"
#define vtI2CErrInit -1
#define vtI2CInitSuccess 0
#define vtI2CTransferFailed -2
#define vtI2CIntPriority 7

// Here is where we define an array of pointers that lets communication occur between the interrupt handler and the rest of the code in this file
static 	vtI2CStruct *devStaticPtr[3];

// I have set this to a large stack size because of (a) using printf() and (b) the depth of function calls
//   for some of the I2C operations -- it is possible this might be reducible
#if PRINTF_VERSION==1
#define i2cSTACK_SIZE		8*configMINIMAL_STACK_SIZE
#else
#define i2cSTACK_SIZE		4*configMINIMAL_STACK_SIZE
#endif

/* The I2C monitor tasks. */
static portTASK_FUNCTION_PROTO( vI2CMonitorTask, pvParameters );

// i2c interrupt handler
static __INLINE void vtI2CIsr(LPC_I2C_TypeDef *devAddr,xSemaphoreHandle *binSemaphore) {
	I2C_MasterHandler(devAddr);
	if (I2C_MasterTransferComplete(devAddr)) {
		static signed portBASE_TYPE xHigherPriorityTaskWoken;
		xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(*binSemaphore,&xHigherPriorityTaskWoken);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
}
// Simply pass on the information to the real interrupt handler above (have to do this to work for multiple i2c peripheral units on the LPC1768
void vtI2C0Isr(void) {
	// Log the I2C status code
	vtITMu8(vtITMPortI2C0IntHandler,((devStaticPtr[0]->devAddr)->I2STAT & I2C_STAT_CODE_BITMASK));
	vtI2CIsr(devStaticPtr[0]->devAddr,&(devStaticPtr[0]->binSemaphore));
}

// Simply pass on the information to the real interrupt handler above (have to do this to work for multiple i2c peripheral units on the LPC1768
void vtI2C1Isr(void) {
	// Log the I2C status code
	vtITMu8(vtITMPortI2C1IntHandler,((devStaticPtr[1]->devAddr)->I2STAT & I2C_STAT_CODE_BITMASK));
	vtI2CIsr(devStaticPtr[1]->devAddr,&(devStaticPtr[1]->binSemaphore));
}
// Simply pass on the information to the real interrupt handler above (have to do this to work for multiple i2c peripheral units on the LPC1768
void vtI2C2Isr(void) {
	vtI2CIsr(devStaticPtr[2]->devAddr,&(devStaticPtr[2]->binSemaphore));
}

// Note: This will startup an I2C thread, once for each call to this routine
int vtI2CInit(vtI2CStruct *devPtr,uint32_t i2cSpeed)
{
	PINSEL_CFG_Type PinCfg;

	int retval = vtI2CInitSuccess;
	switch (devPtr->devNum) {
		case 0: {
			devStaticPtr[0] = devPtr; // Setup the permanent variable for use by the interrupt handler
			devPtr->devAddr = LPC_I2C0;
			// Start with the interrupts disabled *and* make sure we have the priority correct
			NVIC_SetPriority(I2C0_IRQn,vtI2CIntPriority);	
			NVIC_DisableIRQ(I2C0_IRQn);
			// Init I2C pin connect
			PinCfg.OpenDrain = 0;
			PinCfg.Pinmode = 0;
			PinCfg.Funcnum = 1;
			PinCfg.Pinnum = 27;
			PinCfg.Portnum = 0;
			PINSEL_ConfigPin(&PinCfg);
			PinCfg.Pinnum = 28;
			PINSEL_ConfigPin(&PinCfg);
			break;
		}
		case 1: {
			devStaticPtr[1] = devPtr; // Setup the permanent variable for use by the interrupt handler
			devPtr->devAddr = LPC_I2C1;
			// Start with the interrupts disabled *and* make sure we have the priority correct
			NVIC_SetPriority(I2C1_IRQn,vtI2CIntPriority);	
			NVIC_DisableIRQ(I2C1_IRQn);
			// Init I2C pin connect
			PinCfg.OpenDrain = 0;
			PinCfg.Pinmode = 0;
			PinCfg.Funcnum = 3;
			PinCfg.Pinnum = 0;
			PinCfg.Portnum = 0;
			PINSEL_ConfigPin(&PinCfg);
			PinCfg.Pinnum = 1;
			PINSEL_ConfigPin(&PinCfg);
			break;
		}
		default: {
			return(vtI2CErrInit);
			break;
		}
	}

	// Create semaphore to communicate with interrupt handler
	vSemaphoreCreateBinary(devPtr->binSemaphore);
	if (devPtr->binSemaphore == NULL) {
		return(vtI2CErrInit);
	}
	// Need to do an initial "take" on the semaphore to ensure that it is initially blocked
	if (xSemaphoreTake(devPtr->binSemaphore,0) != pdTRUE) {
		// free up everyone and go home
		vQueueDelete(devPtr->binSemaphore);
		return(vtI2CErrInit);
	}

	// Allocate the two queues to be used to communicate with other tasks
	if ((devPtr->inQ = xQueueCreate(vtI2CQLen,sizeof(vtI2CMsg))) == NULL) {
		// free up everyone and go home
		vQueueDelete(devPtr->binSemaphore);
		return(vtI2CErrInit);
	}
	if ((devPtr->outQ = xQueueCreate(vtI2CQLen,sizeof(vtI2CMsg))) == NULL) {
		// free up everyone and go home
		vQueueDelete(devPtr->binSemaphore);
		vQueueDelete(devPtr->outQ);
		return(vtI2CErrInit);
	}

	// Initialize  I2C peripheral
	I2C_Init(devPtr->devAddr, i2cSpeed);

	// Enable  I2C operation
	I2C_Cmd(devPtr->devAddr, ENABLE);

	/* Start the task */
	char taskLabel[8];
	sprintf(taskLabel,"I2C%d",devPtr->devNum);
	if ((retval = xTaskCreate( vI2CMonitorTask, (signed char*) taskLabel, i2cSTACK_SIZE,(void *) devPtr, devPtr->taskPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(retval);
		return(0); // return is just to keep the compiler happy, we will never get here
	} else {
		return vtI2CInitSuccess;
	}
}

// This is the actual task that is run
static portTASK_FUNCTION( vI2CMonitorTask, pvParameters )
{
	// Get the i2c structure for this task/device
	vtI2CStruct *devPtr = (vtI2CStruct *) pvParameters;
	vtI2CMsg msgBuffer;
	uint8_t tmpRxBuf[vtI2CMLen];
	I2C_M_SETUP_Type transferMCfg;
	int i;

	printf("I2C task %d\n",devPtr->devNum);
	for (;;) {
		// wait for a message from another task telling us to send/recv over i2c
		if (xQueueReceive(devPtr->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		//Log that we are processing a message
		vtITMu8(vtITMPortI2CMsg,msgBuffer.msgType);

		// process the messsage and perform the I2C transaction
		transferMCfg.sl_addr7bit = msgBuffer.slvAddr;
		transferMCfg.tx_data = msgBuffer.buf;
		transferMCfg.tx_length = msgBuffer.txLen;
		transferMCfg.rx_data = tmpRxBuf;
		transferMCfg.rx_length = msgBuffer.rxLen;
		transferMCfg.retransmissions_max = 3;
		transferMCfg.retransmissions_count = 0;	 // this *should* be initialized in the LPC code, but is not for interrupt mode
		msgBuffer.status = I2C_MasterTransferData(devPtr->devAddr, &transferMCfg, I2C_TRANSFER_INTERRUPT);
		// Block until the I2C operation is complete -- we *cannot* overlap operations on the I2C bus...
		if (xSemaphoreTake(devPtr->binSemaphore,portMAX_DELAY) != pdTRUE) {
			// something went wrong 
			VT_HANDLE_FATAL_ERROR(0);
		}
		msgBuffer.txLen = transferMCfg.tx_count;
		msgBuffer.rxLen = transferMCfg.rx_count;
		// Now send out a message with the data that was read
		// First, copy over the buffer that was received (if any)
		for (i=0;i<msgBuffer.rxLen;i++) {
			msgBuffer.buf[i] = tmpRxBuf[i];
		}
		// now put a message in the message queue
		if (xQueueSend(devPtr->outQ,(void*)(&msgBuffer),portMAX_DELAY) != pdTRUE) {
			// something went wrong 
			VT_HANDLE_FATAL_ERROR(0);
		} 
	}
}