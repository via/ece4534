#include "maindefs.h"
#include "interrupts.h"
#include "user_interrupts.h"
#include "messages.h"

//----------------------------------------------------------------------------
// Note: This code for processing interrupts is configured to allow for high and
//       low priority interrupts.  The high priority interrupt can interrupt the
//       the processing of a low priority interrupt.  However, only one of each type
//       can be processed at the same time.  It is possible to enable nesting of low
//       priority interrupts, but this code is not setup for that and this nesting is not
//       enabled.

void enable_interrupts()
{
	// Peripheral interrupts can have their priority set to high or low
	// enable high-priority interrupts and low-priority interrupts
	RCONbits.IPEN = 1;
	INTCONbits.GIEH = 1;
	INTCONbits.GIEL = 1;
}

int	in_high_int() {
	return(!INTCONbits.GIEH);
}

int	low_int_active() {
	return(!INTCONbits.GIEL);
}

int	in_low_int() {
	if (INTCONbits.GIEL == 1) {
		return(0);
	} else if (in_high_int()) {
		return(0);
	} else {
		return(1);
	}
}

int	in_main() {
	if ((! in_low_int()) && (! in_high_int())) {
		return(1);
	} else {
		return(0);
	}
}
// Set up the interrupt vectors
void InterruptHandlerHigh();
void InterruptHandlerLow();

#pragma code InterruptVectorLow = 0x18
void
InterruptVectorLow (void)
{
  _asm
    goto InterruptHandlerLow //jump to interrupt routine
  _endasm
}

#pragma code InterruptVectorHigh = 0x08
void
InterruptVectorHigh (void)
{
  _asm
    goto InterruptHandlerHigh //jump to interrupt routine
  _endasm
}

//----------------------------------------------------------------------------
// High priority interrupt routine
// this parcels out interrupts to individual handlers

#pragma code
#pragma interrupt InterruptHandlerHigh

void InterruptHandlerHigh ()
{
	// We need to check the interrupt flag of each enabled high-priority interrupt to
	// see which device generated this interrupt.  Then we can call the correct handler.

	// check to see if we have an I2C interrupt
	if (PIR1bits.SSPIF) {
		// clear the interrupt flag
		PIR1bits.SSPIF = 0;
		// call the handler
		i2c_int_handler();
	}

	// check to see if we have an interrupt on timer 0
	if (INTCONbits.TMR0IF)
    {                                   
      	INTCONbits.TMR0IF = 0;            // clear this interrupt flag
		// call whatever handler you want (this is "user" defined)
      	timer0_int_handler();
    }

	// here is where you would check other interrupt flags.

	// The *last* thing I do here is check to see if we can
	// allow the processor to go to sleep
	// This code *DEPENDS* on the code in messages.c being
	// initialized using "init_queues()" -- if you aren't using
	// this, then you shouldn't have this call here
	SleepIfOkay();
}

//----------------------------------------------------------------------------
// Low priority interrupt routine
// this parcels out interrupts to individual handlers
#pragma code
#pragma interruptlow InterruptHandlerLow

// This works the same way as the "High" interrupt handler
void InterruptHandlerLow()
{
  	// check to see if we have an interrupt on timer 1
  	if (PIR1bits.TMR1IF) {
		PIR1bits.TMR1IF = 0;            //clear interrupt flag
		timer1_int_handler();
	}

  	// check to see if we have an interrupt on USART RX
  	if (PIR1bits.RCIF) {
		PIR1bits.RCIF = 0;            //clear interrupt flag
		uart_recv_int_handler();
	}
}

