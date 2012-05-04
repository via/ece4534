#include "maindefs.h"
#include "interrupts.h"
#include "messages.h"
#include <string.h>
#include "WirelessProtocols/MiWi/MiWi.h"
#include "my_miwi.h"

// The key to making this code safe for interrupts is that
// each queue is filled by only one writer and read by one reader.
// ToMainQueueLow: Writer is a low priority interrupt, Reader is main()
// ToMainQueueHigh: Writer is a high priority interrupt, Reader is main()
// FromMainQueueLow: Writer is main(), Reader is a low priority interrupt
// FromMainQueueHigh: Writer is main(), Reader is a high priority interrupt

void init_queue(msg_queue *qptr)
{
	unsigned char	i;

	qptr->cur_write_ind = 0;
	qptr->cur_read_ind = 0;
	for (i=0;i<MSGQUEUELEN;i++) {
		qptr->queue[i].full = 0;
	}
}

signed char send_msg(msg_queue *qptr,unsigned char length,unsigned char msgtype,void *data)
{
	unsigned char	slot;
	//unsigned char *msgptr = (unsigned char *) data;
	msg	*qmsg;
	size_t tlength = length;

#ifdef DEBUG
	if (length > MSGLEN) {
		return(MSGBAD_LEN);
	} else if (length < 0) {
		return(MSGBAD_LEN);
	}
#endif

	slot = qptr->cur_write_ind;
	qmsg = &(qptr->queue[slot]);
	// if the slot isn't empty, then we should return
	if (qmsg->full != 0) {
		return(MSGQUEUE_FULL);
	}

	// now fill in the message
	qmsg->length = length;
	qmsg->msgtype = msgtype;

/*
	for (i=0;i<length;i++) {
		qptr->queue[slot].data[i] = msgptr[i];
	}
*/

	memcpy(qmsg->data,data,tlength);
	qptr->cur_write_ind = (qptr->cur_write_ind + 1) % MSGQUEUELEN;

	// This *must* be done after the message is completely inserted
	qmsg->full = 1;
	return(MSGSEND_OKAY);
}

signed char	recv_msg(msg_queue *qptr, unsigned char maxlength, unsigned char *msgtype,void *data)
{
	unsigned char	slot;
	//unsigned char	i;
	//unsigned char	retlength;
	//unsigned char *msg = (unsigned char *) data;
	msg	*qmsg;
	size_t tlength;

	// check to see if anything is available
	slot = qptr->cur_read_ind;
	qmsg = &(qptr->queue[slot]);
	if (qmsg->full == 1) {
		// not enough room in the buffer provided
		if (qmsg->length > maxlength) {
			return(MSGBUFFER_TOOSMALL);
		}
		// now actually copy the message
		tlength = qmsg->length;
		memcpy(data,(void *)qmsg->data,tlength);
		/*
		for (i=0;i<qmsg->length;i++) {
			((unsigned char *) data)[i] = qptr->queue[slot].data[i];
		}
		*/
		qptr->cur_read_ind = (qptr->cur_read_ind + 1) % MSGQUEUELEN;
		//retlength = qptr->queue[slot].length;
		(*msgtype) = qmsg->msgtype;
		// this must be done after the message is completely extracted
		qmsg->full = 0;
		return(tlength);
	} else {
		return(MSGQUEUE_EMPTY);
	}
}

#pragma udata msgqueue1
static msg_queue ToMainLow_MQ;
signed char	ToMainLow_sendmsg(unsigned char length,unsigned char msgtype,void *data)
{
#ifdef DEBUG
	if (!in_low_int()) {
		return(MSG_NOT_IN_LOW);
	}
#endif
	return(send_msg(&ToMainLow_MQ,length,msgtype,data));
}

signed char	ToMainLow_recvmsg(unsigned char maxlength,unsigned char *msgtype,void *data)
{
#ifdef DEBUG
	if (! in_main()) {
		return(MSG_NOT_IN_MAIN);
	}
#endif
	return(recv_msg(&ToMainLow_MQ,maxlength,msgtype,data));
}

#pragma udata msgqueue2
static msg_queue ToMainHigh_MQ;
signed char	ToMainHigh_sendmsg(unsigned char length,unsigned char msgtype,void *data)
{
#ifdef DEBUG
	if (! in_high_int()) {
		return(MSG_NOT_IN_HIGH);
	}
#endif
	return(send_msg(&ToMainHigh_MQ,length,msgtype,data));
}
signed char	ToMainHigh_recvmsg(unsigned char maxlength,unsigned char *msgtype,void *data)
{
#ifdef DEBUG
	if (! in_main()) {
		return(MSG_NOT_IN_MAIN);
	}
#endif
	return(recv_msg(&ToMainHigh_MQ,maxlength,msgtype,data));
}

#pragma udata msgqueue3
static msg_queue FromMainLow_MQ;
signed char	FromMainLow_sendmsg(unsigned char length,unsigned char msgtype,void *data)
{
#ifdef DEBUG
	if (! in_main()) {
		return(MSG_NOT_IN_MAIN);
	}
#endif
	return(send_msg(&FromMainLow_MQ,length,msgtype,data));
}
signed char	FromMainLow_recvmsg(unsigned char maxlength,unsigned char *msgtype,void *data)
{
#ifdef DEBUG
	if (!in_low_int()) {
		return(MSG_NOT_IN_LOW);
	}
#endif
	return(recv_msg(&FromMainLow_MQ,maxlength,msgtype,data));
}
#pragma udata msgqueue4
static msg_queue FromMainHigh_MQ;
signed char	FromMainHigh_sendmsg(unsigned char length,unsigned char msgtype,void *data)
{
#ifdef DEBUG
	if (! in_main()) {
		return(MSG_NOT_IN_MAIN);
	}
#endif
	return(send_msg(&FromMainHigh_MQ,length,msgtype,data));
}

signed char	FromMainHigh_recvmsg(unsigned char maxlength,unsigned char *msgtype,void *data)
{
#ifdef DEBUG
	if (!in_high_int()) {
		return(MSG_NOT_IN_HIGH);
	}
#endif
	return(recv_msg(&FromMainHigh_MQ,maxlength,msgtype,data));
}

static unsigned char MQ_Main_Willing_to_block;
void init_queues()
{
	MQ_Main_Willing_to_block = 0;
	init_queue(&ToMainLow_MQ);
	init_queue(&ToMainHigh_MQ);
	init_queue(&FromMainLow_MQ);
	init_queue(&FromMainHigh_MQ);
}

void enter_sleep_mode(void)
{
	OSCCONbits.IDLEN = 1; // set to idle on sleep
//	OSCCONbits.SCS0 = 1;
//	OSCCONbits.SCS1 = 1;
	_asm
		sleep
	_endasm
}

// check if message available
unsigned char check_msg(msg_queue *qptr)
{
	return(qptr->queue[qptr->cur_read_ind].full);
}

// This should only be called from a High Priority Interrupt
void SleepIfOkay()
{
	// we won't sleep if the main isn't willing to block
	if (MQ_Main_Willing_to_block == 0) {
		return;
	}
	// check to see if we are handling a low priority interrupt
	// if so, we are not going to sleep
	if (in_low_int()) {
		return;
	}
	// we know that we are in a high priority interrupt handler
	// but we'll check to make sure and return if we are not
	if (!in_high_int()) {
		return;
	}
	// since we are the only thing executing that could be
	// putting something into a message queue destined for main()
	// we can safely check the message queues now
	//   if they are empty, we'll go to sleep
	if (check_msg(&ToMainHigh_MQ)) {
		return;
	}
	if (check_msg(&ToMainLow_MQ)) {
		return;
	}
	enter_sleep_mode();
}

// only called from "main"
void block_on_To_msgqueues()
{
    int a;
	if (!in_main()) {
		return;
	}

	MQ_Main_Willing_to_block = 1;
	while (1) {
        if (MiMAC_ReceivedPacket()) {
            handlePacket();
            return;
        }
		if (check_msg(&ToMainHigh_MQ)) {
			MQ_Main_Willing_to_block = 0;
			return;
		}
		if (check_msg(&ToMainLow_MQ)) {
			MQ_Main_Willing_to_block = 0;
			return;
		}
		Delay1KTCYx(10);

	}
}