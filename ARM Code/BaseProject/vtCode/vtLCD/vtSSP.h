#ifndef __vtSSPh
#define __vtSSPh
/* ***************************************
* Include file for implementation of SSP interrupt handler and supporting code
****************************************** */

#include "vtUtilities.h"
#include "lpc17xx_ssp.h"

#define vtSSPIntPriority 7

#define vtSSPErrInit -1
#define vtSSPInitSuccess 0

// Define the structure for the buffer
typedef struct __vtSSPIsrData {
	uint32_t length;
	uint32_t tx_cnt;
	void *tx_data;
} vtSSPIsrData;

// The pointer passed into this function *must* always point to a variable that is *not* de-allocated
int vtSSPIsrInit(unsigned short);

void vtSSPStartOperation(vtSSPIsrData *);

portBASE_TYPE vtSSPWaitComplete(portTickType);

void vtSSPIsr(void);
#endif