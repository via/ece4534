#include "maindefs.h"
#include <stdio.h>
#include "timer0_thread.h"
#include "my_adc.h"

// This is a "logical" thread that processes messages from TIMER0
// It is not a "real" thread because there is only the single main thread
// of execution on the PIC because we are not using an RTOS.
int timer0_lthread(timer0_thread_struct *tptr,int msgtype,int length,unsigned char *msgbuffer)
{
	unsigned int *msgval;
    unsigned char adcval;

	msgval = (unsigned int *) msgbuffer;
 
    readADC(&adcval);

	printf("TIMER1: %d %d %d %d\r\n",msgtype,length,(*msgval), adcval);

}