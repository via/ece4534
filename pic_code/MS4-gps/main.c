/* Compile options:  -ml (Large code model) */
#include "maindefs.h"
#include <stdio.h>
#include <usart.h>
#include <i2c.h>
#include <timers.h>
#include "interrupts.h"
#include "messages.h"
#include "my_uart.h"
#include "my_i2c.h"
#include "uart_thread.h"
#include "timer1_thread.h"
#include "timer0_thread.h"
#include "my_adc.h"
#include "i2c_logic.h"

#define MY_ADDRESS_LENGTH 4
#include "ConfigApp.h"
#include "WirelessProtocols/MiWi/MiWi.h"
#include "WirelessProtocols/MCHP_API.h"
ROM char PredefinedPacket[] = {0x01,0x08,0xC4,0xFF,0xFF,0xFF,0xFF,0x07,0x00,0x00,0x00,0x00,0x00};


#pragma config WDT = OFF
#pragma config OSC = HSPLL
#pragma config XINST = ON
//#pragma config WDTEN = OFF
//#pragma config FOSC = HS

void putch(unsigned char a) {
   WriteUSART(a);
}


// This program 
//   (1) prints to the UART and it reads from the UART
//   (2) it "prints" what it reads from the UART to portb (where LEDs are connected)
//   (3) it uses two timers to interrupt at different rates and drive 2 LEDs (on portb)
void main (void)
{
    MAC_TRANS_PARAM parm;
    BYTE txbuf[3];
	DWORD j;
	char c;
	signed char	length;
	unsigned char	msgtype;
	unsigned char last_reg_recvd;
	uart_comm uc;
	i2c_comm ic;
	unsigned char msgbuffer[MSGLEN+1];
	unsigned char i;
	uart_thread_struct	uthread_data; // info for uart_lthread
	timer1_thread_struct t1thread_data; // info for timer1_lthread
	timer0_thread_struct t0thread_data; // info for timer0_lthread

	// set to run really, really fast...
	//OSCCON = 0x6C; // 4 MHz
	OSCTUNEbits.PLLEN = 1; // 4x the clock speed in the previous line

	// initialize my uart recv handling code
	init_uart_recv(&uc);

	// initialize the i2c code
//	init_i2c(&ic);
  //  initADC();

	// init the timer1 lthread
	init_timer1_lthread(&t1thread_data);

	// initialize message queues before enabling any interrupts
	init_queues();

	// set direction for PORTB to output
 // TRISB = 0xFF;
//	LATB = 0x0;

	// set up PORTA for input
/*
	PORTA = 0x0;	// clear the port
	LATA = 0x0;		// clear the output latch
	ADCON1 = 0x0F;	// turn off the A2D function on these pins
	// Only for 40-pin version of this chip CMCON = 0x07;	// turn the comparator off
	TRISA = 0x0F;	// set RA3-RA0 to inputs
*/

    initMiwi();

	// initialize Timers
	OpenTimer0( TIMER_INT_ON & T0_16BIT & T0_SOURCE_INT & T0_PS_1_128);

	

	enable_interrupts();

 
	// Decide on the priority of the enabled peripheral interrupts
	// 0 is low, 1 is high
	// Timer1 interrupt
	IPR1bits.TMR1IP = 1;
	// USART RX interrupt
	IPR1bits.RCIP = 0;
	// I2C interrupt

    PIE1bits.RCIE = 1;
    PIE1bits.TMR1IE = 1;
    InitSymbolTimer();  /* Sets up timer 0 for miwi symbol timer */

	OpenTimer1( TIMER_INT_ON & T1_PS_1_8 & T1_16BIT_RW & T1_SOURCE_INT & T1_OSC1EN_OFF & T1_SYNC_EXT_OFF);

	// configure the hardware i2c device as a slave
//	i2c_configure_slave(I2C_ADDR);


	// configure the hardware USART device
  	OpenUSART( USART_TX_INT_OFF & USART_RX_INT_ON & USART_ASYNCH_MODE & USART_EIGHT_BIT   & 
		USART_CONT_RX & USART_BRGH_LOW, 25);
printf("Starting!\n");

/* Set up MiWi */
if (!MiApp_ProtocolInit(FALSE)) 
    printf("Init failed\n");
if (!MiApp_SetChannel(24))
    printf("SetChannel failed\n");



/* Junk to force an I2C interrupt in the simulator
PIR1bits.SSPIF = 1;
_asm
goto 0x08
_endasm;
*/
	printf("Hello\r\n");
	// loop forever
	// This loop is responsible for "handing off" messages to the subroutines
	// that should get them.  Although the subroutines are not threads, but
	// they can be equated with the tasks in your task diagram if you 
	// structure them properly.
  	while (1) {
		// Call a routine that blocks until either on the incoming
		// messages queues has a message (this may put the processor into
		// an idle mode
		block_on_To_msgqueues();

		// At this point, one or both of the queues has a message.  It 
		// makes sense to check the high-priority messages first -- in fact,
		// you may only want to check the low-priority messages when there
		// is not a high priority message.  That is a design decision and
		// I haven't done it here.
		length = ToMainHigh_recvmsg(MSGLEN,&msgtype,(void *) msgbuffer);
		if (length < 0) {
			// no message, check the error code to see if it is concern
			if (length != MSGQUEUE_EMPTY) {
				printf("Error: Bad high priority receive, code = %x\r\n",
					length);
			}
		} else {
			switch (msgtype) {
				case MSGT_TIMER0: {
					timer0_lthread(&t0thread_data,msgtype,length,msgbuffer);
					break;
				};
				case MSGT_I2C_DATA:
				case MSGT_I2C_DBG: {
				//	printf("I2C Interrupt received %x: ",msgtype);
					for (i=0;i<length;i++) {
						printf(" %x",msgbuffer[i]);
					}
				//	printf("\r\n");
					// keep track of the first byte received for later use
					last_reg_recvd = msgbuffer[0];
          handle_i2c_write(msgbuffer);
					break;
				};
				case MSGT_I2C_RQST: {
				//	printf("I2C Slave Req reg: %d\r\n", last_reg_recvd);
					// The last byte received is the "register" that is trying to be read
					// The response is dependent on the register.
          length = handle_i2c_read(last_reg_recvd, msgbuffer);
          start_i2c_slave_reply(length,msgbuffer);
					break;
				};
				default: {
					printf("Error: Unexpected msg in queue, type = %x\r\n",
						msgtype);
					break;
				};
			};
		}

		// Check the low priority queue
		length = ToMainLow_recvmsg(MSGLEN,&msgtype,(void *) msgbuffer);
		if (length < 0) {
			// no message, check the error code to see if it is concern
			if (length != MSGQUEUE_EMPTY) {
				printf("Error: Bad low priority receive, code = %x\r\n",
					length);
			}
		} else {
			switch (msgtype) {
				case MSGT_TIMER1: {
					timer1_lthread(&t1thread_data,msgtype,length,msgbuffer);
					break;
				};
				case MSGT_OVERRUN:
				case MSGT_UART_DATA: {
					uart_lthread(&uthread_data,msgtype,length,msgbuffer);
					break;
				};
				default: {
					printf("Error: Unexpected msg in queue, type = %x\r\n",
						msgtype);
					break;
				};
			};
		}
 	 }

}
