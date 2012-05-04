#ifndef __my_uart_h
#define __my_uart_h

#include "messages.h"

#define MAXUARTBUF 4
#if (MAXUARTBUF > MSGLEN)
#define MAXUARTBUF MSGLEN
#endif
typedef struct __uart_comm {
	unsigned char buffer[MAXUARTBUF];
	unsigned char	buflen;
} uart_comm;

void init_uart_recv(uart_comm *);
void uart_recv_int_handler(void);
void uart_handle_packet(char *buf, int len);
void uart_send_report(void);


#endif
