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
#include "i2c_logic.h"
#include "my_miwi.h"

#define MY_ADDRESS_LENGTH 4
#include "ConfigApp.h"
#include "WirelessProtocols/MiWi/MiWi.h"
#include "WirelessProtocols/MCHP_API.h"


#pragma config WDTEN = OFF
#pragma config WDTPS = 64 //About 1/4 second
#pragma config OSC = HS
#pragma config XINST = ON

void unlock_rp(void) {
  EECON2 = 0x55;
  EECON2 = 0xAA;
  PPSCONbits.IOLOCK = 0;
}

void initTristates(void) {
    DEBUG0 = 0;
    DEBUG1 = 0;
    MIWI_TX = 0;
    MIWI_STATUS = 0;
    TRISAbits.TRISA1 = 0;
    TRISAbits.TRISA0 = 0;
    TRISAbits.TRISA2 = 0;
    TRISAbits.TRISA3 = 0;  //Set debug output tristates
}

void setLEDDuty(unsigned char b) {

  PR2 = 255; // Timer2 period == about 20kHz period for pwm
  CCPR1L = b; // Set the duty cycle 8 msb's
  RPOR2 = 14; // Set RP2 to be PWM1 A output
  TRISAbits.TRISA5 = 0; // output

  T2CON = 4;  // 0000 0100
  CCP1CON = 0x8F; // 10 00 1111 //Enable PWMing
}

void putch(unsigned char a) {
   Write1USART(a);
}

void initUart(uart_comm *uc) {
    init_uart_recv(uc);
    TRISCbits.TRISC7 = 1;  //RX is input
    TRISCbits.TRISC6 = 0;  //TX is output
    PIE1bits.RC1IE = 1;    //Enable RX interrupt
    IPR1bits.RC1IP = 0;    //Make it low priority
  	Open1USART( USART_TX_INT_OFF & USART_RX_INT_ON & USART_ASYNCH_MODE & USART_EIGHT_BIT   & 
		USART_CONT_RX & USART_BRGH_LOW, 31);
}

void initBeaconTimer(void) {
  	//OpenTimer1( TIMER_INT_ON & T1_PS_1_8 & T1_16BIT_RW & T1_SOURCE_FOSC_4 & T1_OSC1EN_OFF & T1_SYNC_EXT_OFF, 0);
    OpenTimer4(T4_POST_1_16 & T4_PS_1_16);
    PR4 = 243;  
    PIE3bits.TMR4IE = 1;
    

}  
// This program 
//   (1) prints to the UART and it reads from the UART
//   (2) it "prints" what it reads from the UART to portb (where LEDs are connected)
//   (3) it uses two timers to interrupt at different rates and drive 2 LEDs (on portb)
void main (void)
{
    unsigned char buf[5];
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

   
    initTristates();
    unlock_rp();
    //setLEDDuty(20);
    enable_interrupts();    
    initUart(&uc);
    initMiwi();
#ifdef MOBILEUNIT
    
    initBeaconTimer();
#endif
    init_queues();

    
    while(1) {
        /* Clear Watchdog */
        _asm
            CLRWDT
        _endasm

        block_on_To_msgqueues();
        length = ToMainHigh_recvmsg(MSGLEN,&msgtype,(void *) msgbuffer);
        if (length != MSGQUEUE_EMPTY)
            switch (msgtype) {
                case MSGT_TIMER4:
                    uart_send_report(); /* Send miwi message with uart string */
                    break;
#ifndef MOBILEUNIT
                case MSGT_MIWI:
                    handlePacket();
                    break;
#endif
            }
        length = ToMainLow_recvmsg(MSGLEN,&msgtype,(void *) msgbuffer);
        if (length != MSGQUEUE_EMPTY)
            switch (msgtype) {
            case MSGT_OVERRUN:
                DEBUG1 = 1;
                break;
            case MSGT_UART_DATA:
                uart_handle_packet((char *)msgbuffer, length);
                break;
            }
            
    }
}