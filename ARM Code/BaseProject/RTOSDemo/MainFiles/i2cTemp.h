#ifndef I2CTEMP_TASK_H
#define I2CTEMP_TASK_H
#include "vtI2C.h"
#include "lcdTask.h"
#include "calcTask.h"
// Structure used to pass parameters to the task
typedef struct __i2cTempStruct {
	vtI2CStruct *dev0;
	vtI2CStruct *dev1;
	vtLCDStruct *lcdData;
	vtCalcStruct *calcData;
} i2cTempStruct;
void vStarti2cTempTask( unsigned portBASE_TYPE uxPriority, i2cTempStruct *);

#endif