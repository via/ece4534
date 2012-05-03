
#ifndef I2C_LOGIC_H
#define I2C_LOGIC_H

#include "maindefs.h"

#if BOARDNO == 0
#define I2C_ADDR 0x36
#endif

#if BOARDNO == 1
#define I2C_ADDR 0x38
#endif

#if BOARDNO == 2
#define I2C_ADDR 0x3A
#endif

signed char handle_i2c_read(unsigned char reg, unsigned char *buf);
void handle_i2c_write(unsigned char *);


#endif

