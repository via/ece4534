#ifndef FILE_TASK_H
#define FILE_TASK_H
//#include "queue.h"
#include "lcdTask.h"
// Define a data structure that is used to pass parameters to this task
typedef struct __vtFileStruct {
	xQueueHandle inQ;					   	// Queue used to send messages from other tasks to the LCD task to print
	vtLCDStruct *lcdData;
} vtFileStruct;

#define vtFileQLen 5
// Structure used to define the messages that are sent to the LCD thread
//   the maximum length of a message to be printed is the size of the "buf" field below
#define vtFileMLen 64 
typedef struct __vtFileMsg {
	uint8_t	length;	 		 // Length of the message to be printed
	uint8_t buf[vtFileMLen]; // On the way in, message to be sent, on the way out, message received (if any)
	double e_calc;
	double n_calc;
	double e_actual;
	double n_actual;
} vtFileMsg;

void vStartFileTask( unsigned portBASE_TYPE uxPriority, vtFileStruct *);

#endif