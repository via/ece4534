#include "maindefs.h"
#include <stdio.h>
#include "messages.h"
#include "timer1_thread.h"
#include "my_miwi.h"
#include <string.h>

static unsigned char txbuf[101];
static int len = 0;

void init_timer1_lthread(timer1_thread_struct *tptr)
{
	tptr->msgcount = 0;
}

void set_tx_buf(unsigned char *src, int l) {

   strncpy(txbuf, src, l);
   len = l;
}

// This is a "logical" thread that processes messages from TIMER1
// It is not a "real" thread because there is only the single main thread
// of execution on the PIC because we are not using an RTOS.
int timer1_lthread(timer1_thread_struct *tptr,int msgtype,int length,unsigned char *msgbuffer)
{
	signed char	retval;

	tptr->msgcount++;
	// Every tenth message we get from timer1 we
	// send something to the High Priority Interrupt

	if ((tptr->msgcount % 10) == 9) {
	
		retval = FromMainHigh_sendmsg(sizeof(tptr->msgcount),MSGT_MAIN1,(void *)&(tptr->msgcount));

	}
}