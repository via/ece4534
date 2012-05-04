#ifndef CALC_TASK_H
#define CALC_TASK_H
#include "queue.h"
#include "lcdTask.h"
#include "fileTask.h"
// Define a data structure that is used to pass parameters to this task
typedef struct __vtCalcStruct {
	xQueueHandle inQ;					   	// Queue used to send messages from other tasks to the Calc task
	vtLCDStruct *lcdData;
	vtFileStruct *fileData;
} vtCalcStruct;

#define vtCalcQLen 5
// Structure used to define the messages that are sent to the LCD thread
//   the maximum length of a message to be printed is the size of the "buf" field below
#define vtCalcMLen 45
typedef struct __vtCalcMsg {
	uint8_t	length;	 // Length of the message to be printed
	uint8_t buf[vtCalcMLen]; // On the way in, message to be sent, on the way out, message received (if any)
	int latDeg;
	int lonDeg;
	double latMin;
	double lonMin;
} vtCalcMsg;

void vStartCalcTask( unsigned portBASE_TYPE uxPriority, vtCalcStruct *);

#endif