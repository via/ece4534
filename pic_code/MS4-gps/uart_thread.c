#include "maindefs.h"
#include <stdio.h>
#include <string.h>
#include <usart.h>
#include "uart_thread.h"
#include "timer1_thread.h"
#include "my_miwi.h"
// This is a "logical" thread that processes messages from the UART
// It is not a "real" thread because there is only the single main thread
// of execution on the PIC because we are not using an RTOS.

static unsigned char buf[101];
static int curPos = 0;


int uart_lthread(uart_thread_struct *uptr,int msgtype,int length,unsigned char *msgbuffer)
{
    int i;
	if (msgtype == MSGT_OVERRUN) {
       printf("overrun\n");
	} else if (msgtype == MSGT_UART_DATA) {
        
		// print the message (this assumes that the message
		// 		was a printable string)
			msgbuffer[length] = '\0'; // null-terminate the array as a string
            printf("%s ", msgbuffer);
            for (i = 0; i < length; ++i) {
                buf[curPos++] = msgbuffer[i];
                
                if ((curPos == 100) || (msgbuffer[i] == '\r') ) {
                  set_tx_buf(buf, curPos);
                  curPos = 0;
                }
            }
	}

}