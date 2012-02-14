#ifndef LCD_TASK_H
#define LCD_TASK_H
#include "queue.h"
// Define a data structure that is used to pass parameters to this task
typedef struct __vtLCDStruct {
	xQueueHandle inQ;					   	// Queue used to send messages from other tasks to the LCD task to print
} vtLCDStruct;

#define vtLCDQLen 5
// Structure used to define the messages that are sent to the LCD thread
//   the maximum length of a message to be printed is the size of the "buf" field below
#define vtLCDMLen 20 
typedef struct __vtLCDMsg {
	uint8_t	length;	 // Length of the message to be printed
	uint8_t buf[vtLCDMLen]; // On the way in, message to be sent, on the way out, message received (if any)
} vtLCDMsg;

void vStartLCDTask( unsigned portBASE_TYPE uxPriority, vtLCDStruct *);

#endif