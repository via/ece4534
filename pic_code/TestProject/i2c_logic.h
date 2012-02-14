
#ifndef I2C_LOGIC_H
#define I2C_LOGIC_H

#define NODE 1

#if NODE == 0
#define CUR_MEAN 0x40
#define CUR_STDDEV 0x45
#define CUR_CMD_STRING "I am Node 0!       "
#define I2C_ADDR 0x36
#endif

#if NODE == 1
#define CUR_MEAN 0x50
#define CUR_STDDEV 0x55
#define CUR_CMD_STRING "I am Node 1!       "
#define I2C_ADDR 0x38
#endif

#if NODE == 2
#define CUR_MEAN 0x60
#define CUR_STDDEV 0x65
#define CUR_CMD_STRING "I am Node 2!       "
#define I2C_ADDR 0x3A
#endif

signed char handle_i2c_read(unsigned char reg, unsigned char *buf);
void handle_i2c_write(unsigned char *);


#endif

