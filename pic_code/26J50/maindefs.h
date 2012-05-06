#ifndef __maindefs
#define __maindefs

//#include <p18f2680.h>
//#include <p18f45j10.h>
//#include <p18f4620.h>
#include <p18f26j50.h>
// Message type definitions

#define MSGT_TIMER0 10
#define MSGT_TIMER1 11
#define MSGT_TIMER4 14
#define MSGT_MAIN1 20
#define	MSGT_OVERRUN 30
#define MSGT_UART_DATA 31
#define MSGT_I2C_DBG 41
#define	MSGT_I2C_DATA 40
#define MSGT_I2C_RQST 42
#define MSGT_I2C_MASTER_SEND_COMPLETE 43
#define MSGT_I2C_MASTER_SEND_FAILED 44
#define MSGT_I2C_MASTER_RECV_COMPLETE 45
#define MSGT_I2C_MASTER_RECV_FAILED 46
#define MSGT_MIWI 50


#define DEBUG0 LATAbits.LATA2
#define DEBUG1 LATAbits.LATA3
#define MIWI_TX LATAbits.LATA0
#define MIWI_STATUS LATAbits.LATA1

#define BOARDNO 2
//#define MOBILEUNIT
#endif

