#include "maindefs.h"
#include "my_miwi.h"
#include "ConfigApp.h"
#include "WirelessProtocols/MCHP_API.h"
#include "WirelessProtocols/MiWi/MiWi.h"

void initMiwi() {
    TRISC = 0;  /* Set all C output */
    TRISCbits.TRISC4 = 1; /* Except SDI */
    TRISCbits.TRISC7 = 1; /* Uart RX */
    TRISCbits.TRISC1 = 1; /* Wake */

    LATC = 0xfe;   /*All high except CS */
    LATCbits.LATC3 = 0; /*SCK low */

    TRISBbits.TRISB0 = 1;
    LATB = 0;
    PORTB = 0;

    SSPCON1 = 0x0;       /* Set up SPI core */
	SSPCON1bits.SSPM = 0x2;
	SSPCON1bits.CKP = 0;
	SSPSTATbits.CKE = 1;
	SSPSTATbits.SMP = 1;

    SSPCON1bits.SSPEN = 1; /*Enable SPI interrupt */

    RFIF = 0; /* Enable int0 and clear its interrupt flag */
    RFIE = 1;

    INTCON2bits.INTEDG0 = 0;
}

void send_report(unsigned char type, int len, void *data) {
    unsigned char *b = data;
	MiApp_FlushTx();
    MiApp_WriteData(type);
    MiApp_WriteData(0x00);
    while (len--) {
	 MiApp_WriteData(*b++);
    }

    if (MiApp_BroadcastPacket(FALSE))
        putch('.');
    else
        putch('-');
}

void handle_miwi_packet() {

    printf("Received packet RSSI: %d Len: %d\n", MACRxPacket.RSSIValue,
              MACRxPacket.PayloadLen);
    MACRxPacket.Payload[MACRxPacket.PayloadLen] = '\0';
    printf("Received: [%s]\n", MACRxPacket.Payload + 13);

}