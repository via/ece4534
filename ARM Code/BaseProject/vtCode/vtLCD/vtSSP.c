/******************************************************************************/
// The following is an example of an acceptable implementation of an interrupt handler
// that does not rely on globally accessible variables.
//
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "semphr.h"
#include "vtSSP.h"

// Often, an interrupt handler needs some type of initilization data from the "rest" of the program.
//   This initialization data does not change over time and is not for ongoing communication.  We'll use
//   a structure defined for that type of information -- and you should *always* do the same in your programs.
//
// Here is a structure definition used for initialization of our interrupt handler.
// If we wanted to use this anywhere else (which might not be unreasonable), we would put this in a .h
//   file for inclusion by other programs.  We would do the same for functions we expect other programs to use.
typedef struct __vtSSPIsrStruct {
	unsigned short unitNum; // Is it SSP 0 or 1
	xSemaphoreHandle binSemaphore; // Binary semaphore used for coordination with tasks
	LPC_SSP_TypeDef *SSPx; // Pointer to the SSP module we are actually using
	vtSSPIsrData *dataSetup; // temporary -- will replace
} vtSSPIsrStruct;
// Now that we have defined the structure, we will allocate a variable for it.  
//   The static declaration ensures that this variable is *not* visible outside of this file
static vtSSPIsrStruct initSSPdata;

/* *************************
Private Functions
************************** */
// Declare it static so that it cannot be called by any routine outside of this file
static unsigned char vtSSPFastWriteBuffer(LPC_SSP_TypeDef *SSPx, vtSSPIsrData *dCfg)
{
	uint8_t *dptr;
	uint32_t cnt;
	uint32_t status;


	cnt = dCfg->length - dCfg->tx_cnt;
	dptr = ((uint8_t *) dCfg->tx_data) + dCfg->tx_cnt;
	//if (SSPx->SR & SSP_SR_TFE) {
	//	if (cnt > 8) cnt = 8;
	//} else {
		if (cnt > 4) cnt = 4;
//	}
 	dCfg->tx_cnt += cnt;
	for (;cnt>0;cnt--) {
		// Write data to buffer
		SSPx->DR = (*dptr);
		dptr++;
	}

	// Are we done?
	if (dCfg->tx_cnt != dCfg->length) {
		status = SSPx->MIS;
		return(0);
	} else {
		// make sure the send queue has really emptied and the module is no longer busy
		status = SSPx->SR;
		while ((!(status & SSP_SR_TFE)) || (status & SSP_SR_BSY)){
			status = SSPx->SR;
		}
		return(1);
	}
}

/* *************************
Public Functions
************************** */

// The pointer passed into this function *must* always point to a variable that is *not* de-allocated
int vtSSPIsrInit(unsigned short unitNum) {
	initSSPdata.unitNum = unitNum;
	switch (unitNum) {
		case 0: {
			initSSPdata.SSPx = LPC_SSP0;
			break;
		}
		case 1: {
			initSSPdata.SSPx = LPC_SSP1;
			break;
		}
		default: {
			return(vtSSPErrInit);
			break;
		}
	}
	initSSPdata.dataSetup = NULL;
	vSemaphoreCreateBinary(initSSPdata.binSemaphore);
	if (initSSPdata.binSemaphore == NULL) {
		return(vtSSPErrInit);
	}
	// Need to do an initial "take" on the semaphore to ensure that it is initially blocked
	if (xSemaphoreTake(initSSPdata.binSemaphore,0) != pdTRUE) {
		// free up everyone and go home
		vQueueDelete(initSSPdata.binSemaphore);
		return(vtSSPErrInit);
	}
	return(vtSSPInitSuccess);
}

// Call this function to begin an interrupt driven write of the data buffer
void vtSSPStartOperation(vtSSPIsrData *dptr)
{
	// Note that the sequence is the following:
	// 1. Set any data that will be used by the interrupt handler
	// 2. Set the interrupt priority -- this *must* be done and must be between configKERNEL_INTERRUPT_PRIORITY 	( 31 << (8 - configPRIO_BITS) )
	//    and configMAX_SYSCALL_INTERRUPT_PRIORITY (defined in freertosconfig.h).  The lower the priority number assigned
	//    to an interrupt, the higher the "priority" the interrupt is given over other interrupts -- i.e., "0" has
	//    the highest priority.
	// 3. Clear any pending interrupts of this type via the NVIC (Nested Vector Interrupt Controller)
	// 4. Enable the interrupt via the NVIC
	// 5. Unmask any interrupts in the SSP module
	initSSPdata.dataSetup = dptr;
	initSSPdata.dataSetup->tx_cnt = 0;
	if (initSSPdata.unitNum == 0) {
		NVIC_SetPriority(SSP0_IRQn,vtSSPIntPriority);
		NVIC_ClearPendingIRQ(SSP0_IRQn);	
		NVIC_EnableIRQ(SSP0_IRQn);
	} else {
		NVIC_SetPriority(SSP1_IRQn,vtSSPIntPriority);
		NVIC_ClearPendingIRQ(SSP1_IRQn);	
		NVIC_EnableIRQ(SSP1_IRQn);
	}
	initSSPdata.SSPx->IMSC = SSP_INTCFG_TX;
}

// Wait on completion of an interrupt driven SSP write that was started with vtSSPStartOperation()
portBASE_TYPE vtSSPWaitComplete(portTickType delay)
{
	portBASE_TYPE retVal;
	retVal = xSemaphoreTake(initSSPdata.binSemaphore,delay);
	NVIC_DisableIRQ(SSP1_IRQn);
	return(retVal);
}

// This function assumes that SSP_isrInit() has already been successfully executed
// This function *only* handles the TX side of things and completely ignores RX
void vtSSPIsr(void) {
	// Mask out interrupts from SSP
	initSSPdata.SSPx->IMSC = 0;
	// Write data to the SSP module
	if (vtSSPFastWriteBuffer(initSSPdata.SSPx,initSSPdata.dataSetup)) {
		// We have completed writing the entire buffer
		//   Signal that the buffer is now free for other use via the Semaphore
		//   All four of the following lines are done as per the FreeRTOS API requirements
		static signed portBASE_TYPE xHigherPriorityTaskWoken;
		xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(initSSPdata.binSemaphore,&xHigherPriorityTaskWoken);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	} else {
		// We have not yet finished writing the buffer, so unmask the SSP TX interrupt
		initSSPdata.SSPx->IMSC = SSP_INTCFG_TX;
	}
}
