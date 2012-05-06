
#include <string.h>
#include <stdio.h>
#include <usart.h>


#include "i2c_logic.h"
#include "my_miwi.h"


signed char handle_i2c_read(unsigned char reg, unsigned char *buf) {
  struct location *loc = get_location();
  switch(reg) {
    case 0x00: 
      get_all_rssi(buf);
      return 3;
    case 0x01:
      memcpy(buf, (void*)loc, sizeof(struct location));
      return sizeof(struct location);
    case 0x02:
      buf[0] = 0x10 + BOARDNO;
      return 1;
    default:
      return 0;
    }

}

void handle_i2c_write(unsigned char *buf) {

  unsigned char reg = buf[0];

  switch (reg) {
    case 0x01:
      break;
    default:
      break;
  }

}
