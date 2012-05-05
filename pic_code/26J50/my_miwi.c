#include "maindefs.h"
#include <timers.h>
#include "my_miwi.h"
#include "ConfigApp.h"
#include "messages.h"
#include "WirelessProtocols/MCHP_API.h"
#include "WirelessProtocols/MiWi/MiWi.h"
#include "nmeaparser.h"

static char cur_gps_string[120];
static char final_rssi_values[3];
static char rssi_queue[8];
static int cur_rssi_pos = 0;
struct location l = {0};

void initMiwi() {
  
    TRISCbits.TRISC0 = 1; /* SDI input*/
    TRISCbits.TRISC1 = 0; /* SDO output */
    TRISCbits.TRISC2 = 0; /* Clock output */
    PHY_CS_TRIS = 0;
    PHY_CS = 1;
    RF_INT_TRIS = 1; /* Interrupt input */
    PHY_RESETn_TRIS = 0; /* reset output */
    PHY_WAKE_TRIS = 0; /* Wake output */
    PHY_RESETn = 1;
    PHY_WAKE = 1;
    SPI_SCK = 0; /*SCK inactive */

    RPOR12 = 9; /*RP12 is MSSP2 SDO */
    RPOR13 = 10; /* RP13 is MSSP2 SCK */
    RPINR21 = 11; /* RP11 is MSSP2 SDI */
    RPINR22 = 13; /*RP13 also SCK in */

    SSP2CON1 = 0x0;       /* Set up SPI core */
	SSP2CON1bits.SSPM = 0x2;
	SSP2CON1bits.CKP = 0;
	SSP2STATbits.CKE = 1;
	SSP2STATbits.SMP = 0;

    SSP2CON1bits.SSPEN = 1; /*Enable SPI controller */

    INTCON2bits.INTEDG0 = 0;
    
    RFIF = 0; /* Enable int0 and clear its interrupt flag */
    RFIE = 1;



    InitSymbolTimer();


    MIWI_STATUS = MiApp_ProtocolInit(FALSE);

    MiApp_SetChannel(24);
    MiMAC_SetPower(36);
   

    
}

void send_report(unsigned char type, int len, void *data) {
    unsigned char *b = data;
    MIWI_TX = 1;
	MiApp_FlushTx();
    MiApp_WriteData(type);
    MiApp_WriteData(0x00);
    while (len--) {
	 MiApp_WriteData(*b++);
    }

    MiApp_BroadcastPacket(FALSE);
    MIWI_TX = 0;

}

void handlePacket(void) {

/* For whateve rreason, 0x0D is the offset into the payload of the actual payload */
        
        static int txcounter = 0;
        if (MACRxPacket.Payload[0x0D] == '$') {/* This is a GPS string from Mobile Board */
            if (MACRxPacket.PayloadLen - 0x0D < 120) {
                memcpy((void*)cur_gps_string, (void*)&MACRxPacket.Payload[0x0D], MACRxPacket.PayloadLen - 0x0D);
                cur_gps_string[MACRxPacket.PayloadLen - 0x0D] = '\0';
                if (txcounter++ % 8 == 0) {
                    parse_nmea(&l, cur_gps_string);
                }
                record_new_rssi(MACRxPacket.RSSIValue);
            }
        } else if ( MACRxPacket.Payload[0x0D] == 0x00 ||
                    MACRxPacket.Payload[0x0D] == 0x01 ||
                    MACRxPacket.Payload[0x0D] == 0x02 ) {
            /* This is an RSSI value from another board */
            record_other_rssi(MACRxPacket.Payload[0x0D], MACRxPacket.Payload[0x0E]);
        }               
        MiMAC_DiscardPacket();
    
 }



void record_new_rssi(char rssi) {
    int i;
    unsigned int total = 0;
    char msgbuf[2];

    rssi_queue[cur_rssi_pos++] = rssi;
    if (cur_rssi_pos == 8) {
        cur_rssi_pos = 0;
        
        for (i = 0; i < 8; ++i) {
            total += rssi_queue[i];
        }
        total /= 8;
        final_rssi_values[BOARDNO] = total;
        msgbuf[0] = BOARDNO;
        msgbuf[1] = total;
        send_report(0, 2, msgbuf);
    }
}

void record_other_rssi(char boardID, char rssi) {
    final_rssi_values[boardID] = rssi;

}

void get_all_rssi(unsigned char* msgbuffer) {
    msgbuffer[0] = final_rssi_values[0];
    msgbuffer[1] = final_rssi_values[1];
    msgbuffer[2] = final_rssi_values[2];
}

struct location * get_location() {
    return &l;
}