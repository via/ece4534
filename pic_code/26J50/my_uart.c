#include "maindefs.h"
#include <usart.h>
#include "my_uart.h"
#include <string.h>
#include "my_miwi.h"

static uart_comm *uc_ptr;

static char linebuf[120];
static char txbuf[120];
static int linebufpos = 0;

void uart_recv_int_handler()
{
    char b;
	if (DataRdy1USART()) {
        b = Read1USART();
		uc_ptr->buffer[uc_ptr->buflen] = b;
		uc_ptr->buflen++;
		// check if a message should be sent
		if (uc_ptr->buflen == MAXUARTBUF || b == 0xA) {
            
			ToMainLow_sendmsg(uc_ptr->buflen,MSGT_UART_DATA,(void *) uc_ptr->buffer);
			uc_ptr->buflen = 0;
		}
	}
	if (USART1_Status.OVERRUN_ERROR == 1) {
		// we've overrun the USART and must reset
		// send an error message for this
		RCSTA1bits.CREN = 0;
		RCSTA1bits.CREN = 1;
		ToMainLow_sendmsg(0,MSGT_OVERRUN,(void *) 0);
	}
}

void init_uart_recv(uart_comm *uc)
{	uc_ptr = uc;
	uc_ptr->buflen = 0;
    txbuf[0] = '\0';
}

void uart_send_report(void) {
    if (txbuf[0] == '$')
        send_report(0, strlen(txbuf), txbuf);
}

void uart_handle_packet(char *buf, int len) {
   if (linebufpos + len > 120) {
       DEBUG1 = 1;
   }

   memcpy((void *)linebuf + linebufpos, (void *)buf, len);
   linebufpos += len;

   if (buf[len - 1] == 0xA) {
       if ( linebuf[3] == 'G' &&
            linebuf[4] == 'G' &&  /* We only want to send location information */
            linebuf[5] == 'A' ) {
           memcpy((void *)txbuf, (void *)linebuf, linebufpos);
           txbuf[linebufpos] = '\0';
       }
       linebufpos = 0;      

   }

}