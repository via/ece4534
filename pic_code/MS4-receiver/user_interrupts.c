// This is where the "user" interrupts handlers should go
// The *must* be declared in "user_interrupts.h"

#include "maindefs.h"
#include <timers.h>
#include "user_interrupts.h"
#include "messages.h"

// A function called by the interrupt handler
// This one does the action I wanted for this program on a timer0 interrupt
void timer0_int_handler()
{
	unsigned int val;
	int	length, msgtype;

	// toggle an LED
    LATBbits.LATB0 = !LATBbits.LATB0; 
	// reset the timer
	WriteTimer0(0);
	// try to receive a message and, if we get one, echo it back
	length = FromMainHigh_recvmsg(sizeof(val),&msgtype,(void *)&val);
	if (length == sizeof(val)) {
		ToMainHigh_sendmsg(sizeof(val),MSGT_TIMER0,(void *) &val);
	} 
}

// A function called by the interrupt handler
// This one does the action I wanted for this program on a timer1 interrupt
void timer1_int_handler()
{
	unsigned int result;

	// read the timer and then send an empty message to main()
	result = ReadTimer1();
	ToMainLow_sendmsg(0,MSGT_TIMER1,(void *) 0);
	
	// reset the timer
	WriteTimer1(0);
}