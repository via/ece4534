#include "maindefs.h"
#include <i2c.h>
#include "my_i2c.h"
#include <stdio.h>
static i2c_comm *ic_ptr;

void	start_i2c_slave_reply(unsigned char length,unsigned char *msg)
{


	for (ic_ptr->outbuflen=0;ic_ptr->outbuflen<length;ic_ptr->outbuflen++) {
		ic_ptr->outbuffer[ic_ptr->outbuflen] = msg[ic_ptr->outbuflen];
	}
	ic_ptr->outbuflen = length;
	ic_ptr->outbufind = 1; // point to the second byte to be sent

	// put the first byte into the I2C peripheral
	SSPBUF = ic_ptr->outbuffer[0];
	// we must be ready to go at this point, because we'll be releasing the I2C
	// peripheral which will soon trigger an interrupt
	SSPCON1bits.CKP = 1;
	
}

// an internal subroutine used in the slave version of the i2c_int_handler
void	handle_start(unsigned char data_read)
{
	ic_ptr->event_count = 1;
	ic_ptr->buflen = 0;
	// check to see if we also got the address	
	if (data_read) {
		if (SSPSTATbits.D_A == 1) {
			// this is bad because we got data and
			// we wanted an address
			ic_ptr->status = I2C_IDLE;
			ic_ptr->error_count++;
			ic_ptr->error_code = I2C_ERR_NOADDR;
		} else {
			if (SSPSTATbits.R_W == 1) {
				ic_ptr->status = I2C_SLAVE_SEND;
			} else {
				ic_ptr->status = I2C_RCV_DATA;
			}
		}
	} else {
		ic_ptr->status = I2C_STARTED;
	}
}

// this is the interrupt handler for i2c -- it is currently built for slave mode
// -- to add master mode, you should determine (at the top of the interrupt handler)
//    which mode you are in and call the appropriate subroutine.  The existing code
//    below should be moved into its own "i2c_slave_handler()" routine and the new
//    master code should be in a subroutine called "i2c_master_handler()"
void i2c_int_handler()
{
	unsigned char	i2c_data;
	unsigned char	data_read = 0;
	unsigned char	data_written = 0;
	unsigned char	msg_ready = 0;
	unsigned char	msg_to_send = 0;
	unsigned char	overrun_error = 0;
	unsigned char	error_buf[3];

	// clear SSPOV
	if (SSPCON1bits.SSPOV == 1) {
		SSPCON1bits.SSPOV = 0;
		// we failed to read the buffer in time, so we know we
		// can't properly receive this message, just put us in the
		// a state where we are looking for a new message
		ic_ptr->status = I2C_IDLE;
		overrun_error = 1;
		ic_ptr->error_count++;
		ic_ptr->error_code = I2C_ERR_OVERRUN;
	}
	// read something if it is there
	if (SSPSTATbits.BF == 1) {
		i2c_data = SSPBUF;
		data_read = 1;
	}

	// toggle an LED
   	//LATBbits.LATB2 = !LATBbits.LATB2;

	if (!overrun_error) {
		switch(ic_ptr->status) {
			case	I2C_IDLE: {
				// ignore anything except a start
				if (SSPSTATbits.S == 1) {
					handle_start(data_read);	
					// if we see a slave read, then we need to handle it here
					if (ic_ptr->status == I2C_SLAVE_SEND) {
						data_read = 0;
						msg_to_send = 1;
					}
				}	
				break;
			}
			case	I2C_STARTED: {
				// in this case, we expect either an address or a stop bit
				if (SSPSTATbits.P == 1) {
					// we need to check to see if we also read an
					// address (a message of length 0)
					ic_ptr->event_count++;
					if (data_read) {
						if (SSPSTATbits.D_A == 0) {
							msg_ready = 1;
						} else {
							ic_ptr->error_count++;
							ic_ptr->error_code = I2C_ERR_NODATA;
						}
					}
					ic_ptr->status = I2C_IDLE;
				} else if (data_read) {
					ic_ptr->event_count++;
					if (SSPSTATbits.D_A == 0) {
						if (SSPSTATbits.R_W == 0) { // slave write
							ic_ptr->status = I2C_RCV_DATA;
						} else { // slave read
							ic_ptr->status = I2C_SLAVE_SEND;
							msg_to_send = 1;
							// don't let the clock stretching bit be let go
							data_read = 0;
						}
					} else {
						ic_ptr->error_count++;
						ic_ptr->status = I2C_IDLE;
						ic_ptr->error_code = I2C_ERR_NODATA;
					}
				}
				break;
			}
			case I2C_SLAVE_SEND: {
				if (ic_ptr->outbufind < ic_ptr->outbuflen) {
					SSPBUF = ic_ptr->outbuffer[ic_ptr->outbufind];
					ic_ptr->outbufind++;
					data_written = 1;
				} else {
					// we have nothing left to send
					ic_ptr->status = I2C_IDLE;
				}
				break;
			}
			case I2C_RCV_DATA: {
				// we expect either data or a stop bit or a (if a restart, an addr)
				 if (SSPSTATbits.P == 1) {
					// we need to check to see if we also read data
					ic_ptr->event_count++;
					if (data_read) {
						if (SSPSTATbits.D_A == 1) {
							ic_ptr->buffer[ic_ptr->buflen] = i2c_data;
							ic_ptr->buflen++;
							msg_ready = 1;
						} else {
							ic_ptr->error_count++;
							ic_ptr->error_code = I2C_ERR_NODATA;
							ic_ptr->status = I2C_IDLE;
						}
					} else {
						msg_ready = 1;
					}
					ic_ptr->status = I2C_IDLE;
				} else if (data_read) {
					ic_ptr->event_count++;
					if (SSPSTATbits.D_A == 1) {
						ic_ptr->buffer[ic_ptr->buflen] = i2c_data;
						ic_ptr->buflen++;
					} else /* a restart */ {
						if (SSPSTATbits.R_W == 1) {
							ic_ptr->status = I2C_SLAVE_SEND;
							msg_ready = 1;
							msg_to_send = 1;
							// don't let the clock stretching bit be let go
							data_read = 0;
						} else { /* bad to recv an address again, we aren't ready */
							ic_ptr->error_count++;
							ic_ptr->error_code = I2C_ERR_NODATA;
							ic_ptr->status = I2C_IDLE;
						}	
					}
				}
				break;
			}
		}
	}

	// release the clock stretching bit (if we should)
	if (data_read || data_written) {
		// release the clock
		if (SSPCON1bits.CKP == 0) {
			SSPCON1bits.CKP = 1;
		}
	}

	// must check if the message is too long, if 
	if ((ic_ptr->buflen > MAXI2CBUF-2) && (!msg_ready)) {
		ic_ptr->status = I2C_IDLE;
		ic_ptr->error_count++;
		ic_ptr->error_code = I2C_ERR_MSGTOOLONG;
	}

	if (msg_ready) {
		ic_ptr->buffer[ic_ptr->buflen] = ic_ptr->event_count;
		ToMainHigh_sendmsg(ic_ptr->buflen+1,MSGT_I2C_DATA,(void *) ic_ptr->buffer);
		ic_ptr->buflen = 0;
	} else if (ic_ptr->error_count >= I2C_ERR_THRESHOLD) {
		error_buf[0] = ic_ptr->error_count;
		error_buf[1] = ic_ptr->error_code;
		error_buf[2] = ic_ptr->event_count;
		ToMainHigh_sendmsg(sizeof(unsigned char)*3,MSGT_I2C_DBG,(void *) error_buf);
		ic_ptr->error_count = 0;
	}
	if (msg_to_send) {
		// send to the queue to *ask* for the data to be sent out
		ToMainHigh_sendmsg(0,MSGT_I2C_RQST,(void *)ic_ptr->buffer);
		msg_to_send = 0;
	}
}

// set up the data structures for this i2c code
// should be called once before any i2c routines are called
void init_i2c(i2c_comm *ic)
{	ic_ptr = ic;
	ic_ptr->buflen = 0;
	ic_ptr->event_count = 0;
	ic_ptr->status = I2C_IDLE;
	ic_ptr->error_count = 0;
}

// setup the PIC to operate as a slave
// the address must include the R/W bit
void i2c_configure_slave(unsigned char addr) {

	// ensure the two lines are set for input (we are a slave)
	TRISBbits.TRISB5=1;
	TRISBbits.TRISB4=1;
	// set the address
	SSP1ADD = addr;
	//OpenI2C(SLAVE_7,SLEW_OFF); // replaced w/ code below
	SSP1STAT = 0x0;
	SSP1CON1 = 0x0;
	SSP1CON2 = 0x0;
	SSP1CON1 |= 0x0E;  // enable Slave 7-bit w/ start/stop interrupts
	SSP1STAT |= SLEW_OFF;
	// enable clock-stretching 
	SSP1CON2bits.SEN = 1;
	SSP1CON1 |= SSPENB;
    PIR1bits.SSP1IF = 0;
    PIE1bits.SSP1IE = 1;
	// end of i2c configure
}