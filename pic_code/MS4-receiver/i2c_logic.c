
#include <string.h>
#include <stdio.h>
#include <usart.h>


#include "i2c_logic.h"

static unsigned char cur_mean = CUR_MEAN;
static unsigned char cur_stddev = CUR_STDDEV;
static unsigned char cur_power = 0;



signed char handle_i2c_read(unsigned char reg, unsigned char *buf) {
  int val;
  switch(reg) {
    case 0x00: 
      readADC(&val);
 /*     buf[0] = cur_mean;
      buf[1] = cur_stddev;  */
      buf[0] = val & 0xFF;
      buf[1] = val >> 8;
      return 2;
    case 0x01:
      buf[0] = cur_power;
      return 1;
    case 0x02:
      strcpypgm2ram(buf, CUR_CMD_STRING);
      return 10;
    default:
      return 0;
    }

}

void handle_i2c_write(unsigned char *buf) {

  unsigned char reg = buf[0];

  switch (reg) {
    case 0x01:
      cur_power = buf[1];
      break;
    default:
      break;
  }

}
