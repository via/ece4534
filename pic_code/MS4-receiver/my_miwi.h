#ifndef MY_MIWI_H
#define MY_MIWI_H


void send_report(unsigned char type, int len, void *data);
void initMiwi();
void handle_miwi_packet();

#endif
