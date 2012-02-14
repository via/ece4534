#ifndef __vtI2Ch
#define __vtI2Ch
/* include files. */
#include "lpc17xx_i2c.h"
#include "vtUtilities.h"
#include "FreeRTOS.h"
#include "projDefs.h"
#include "semphr.h"

#define vtI2CErrInit -1
#define vtI2CInitSuccess 0
#define vtI2CIntPriority 7
#define vtI2CQLen 10
#define vtI2CIncompleteOp 0x11


// Structure used to define the messages that are sent to/from the I2C thread
//   the maximum length of a message to be sent/received over I2C is the size of the "buf" field below
#define vtI2CMLen 64 
typedef struct __vtI2CMsg {
	uint8_t msgType; // A field you will likely use in your communications between processors (and for debugging)
	uint8_t slvAddr; // Address of the device to whom the message is being sent (or was sent)
	uint8_t	rxLen;	 // Length of the message you *expect* to receive (or, on the way back, the length that *was* received)
	uint8_t txLen;   // Length of the message you want to sent (or, on the way back, the length that *was* sent)
	uint8_t status;  // status of the completed operation -- I've not done anything much here, you probably should...
	uint8_t buf[vtI2CMLen]; // On the way in, message to be sent, on the way out, message received (if any)
} vtI2CMsg;

// Structure that is used to define the operate of an I2C peripheral using the vtI2C routines
//   It should be initialized by vtI2CInit() and then not changed by anything...
typedef struct __vtI2CStruct {
	uint8_t devNum;	  						// Number of the I2C peripheral (0,1,2 on the 1768)
	LPC_I2C_TypeDef *devAddr;	 			// Memory address of the I2C peripheral
	unsigned portBASE_TYPE taskPriority;   	// Priority of the I2C task
	xSemaphoreHandle binSemaphore;		   	// Semaphore used between I2C task and I2C interrupt handler
	xQueueHandle inQ;					   	// Queue used to send messages from other tasks to the I2C task
	xQueueHandle outQ;						// Queue used by the I2C task to send out results
} vtI2CStruct;

// i2c interrupt handler for I2C0 device on 1768
void vtI2C0Isr(void);
// i2c interrupt handler for I2C1 device on 1768
void vtI2C1Isr(void);
// i2c interrupt handler for I2C2 device on 1768
void vtI2C2Isr(void);

// Must be called for each I2C device initialized (0, 1, or 2) and used
int vtI2CInit(vtI2CStruct *devPtr,uint32_t i2cSpeed);

// A simple routine to use for filling out and sending a message to the I2C thread
//   You may want to make your own versions of these as they are not suited to all purposes
static __INLINE portBASE_TYPE vtI2CEnQ(vtI2CStruct *dev,uint8_t msgType,uint8_t slvAddr,uint8_t txLen,const uint8_t *txBuf,uint8_t rxLen)
{
	vtI2CMsg msgBuf;
	int i;

    msgBuf.slvAddr = slvAddr;
	msgBuf.msgType = msgType;
	msgBuf.rxLen = rxLen;
	if (msgBuf.rxLen > vtI2CMLen) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	msgBuf.txLen = txLen;
	if (msgBuf.txLen > vtI2CMLen) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	for (i=0;i<msgBuf.txLen;i++) {
		msgBuf.buf[i] = txBuf[i];
	}
	return(xQueueSend(dev->inQ,(void *) (&msgBuf),portMAX_DELAY));
}

// A simple routine to use for retrieving a message from the I2C thread
static __INLINE portBASE_TYPE vtI2CDeQ(vtI2CStruct *dev,uint8_t maxRxLen,uint8_t *rxBuf,uint8_t *rxLen,uint8_t *status)
{
	vtI2CMsg msgBuf;
	int i;

	if (xQueueReceive(dev->outQ,(void *) (&msgBuf),portMAX_DELAY) != pdTRUE) {
		return(pdFALSE);
	}
	(*status) = msgBuf.status;
	(*rxLen) = msgBuf.rxLen;
	if (msgBuf.rxLen > maxRxLen) msgBuf.rxLen = maxRxLen;
	for (i=0;i<msgBuf.rxLen;i++) {
		rxBuf[i] = msgBuf.buf[i];
	}
	return(pdTRUE);
}
#endif
