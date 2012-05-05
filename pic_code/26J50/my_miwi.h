#ifndef MY_MIWI_H
#define MY_MIWI_H

#include "nmeaparser.h"

void send_report(unsigned char type, int len, void *data);
void initMiwi(void);
void handlePacket(void);
void record_new_rssi(char);
void record_other_rssi(char, char);
void get_all_rssi(unsigned char* msgbuffer);
struct location *get_location(void);

#endif
