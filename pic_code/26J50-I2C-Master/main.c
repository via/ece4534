/* Compile options:  -ml (Large code model) */
#define I2C_SCL TRISBbits.TRISB4
#define I2C_SDA TRISBbits.TRISB5

#include "maindefs.h"
#include <i2c.h>

void wait_a_little(){
	INTCON &= 0b11111011;
	INTCON |= 0x20;
	TMR0L = 0xFF - 120;
	T0CON = 0b11101000;
	while( ~(INTCON & 0x04) );
	INTCON &= 0b11011011;
}

#define SDAset(n) PORTBbits.RB5=n;LATBbits.LATB5=n
#define SDA PORTBbits.RB5
#define SCLset(n) PORTBbits.RB4=n;LATBbits.LATB4=n
#define SCL PORTBbits.RB4

/*****************************************************/
const char Slave = 0x7C;
const char Comsend = 0x00;
const char Datasend = 0x40;
const char Wake = 0x30;
const char Function = 0x31;
const char Internalosc = 0x14;
const char Powercontrol = 0x56;
const char Followercontrol = 0x6D;
const char Contrast = 0x25;
const char Displayon = 0x0C;
const char Entrymode = 0x06;
const char Clear = 0x01;
const char Line2 = 0xC0;
/*****************************************************/
void I2C_out(unsigned char j)
//I2C Output
{
	WriteI2C1( j );
}
/*****************************************************/
void I2C_Start(void)
{
	StartI2C1();
}
/*****************************************************/
void I2C_Stop(void)
{
	StopI2C1();
}
/*****************************************************/
void Show(unsigned char *text)
{
	int n;
	I2C_Start();
	I2C_out(Slave);
	I2C_out(Datasend);
	for(n=0;n<16;n++){
		I2C_out(*text);
		++text;
	}
	I2C_Stop();
}
/****************************************************
*
Initialization For ST7032i
*
*****************************************************/
void init_LCD()
{
	I2C_Start();
	I2C_out(Slave);
	I2C_out(Comsend);
	I2C_out(0x31);
	I2C_out(Internalosc);
	I2C_out(Contrast);
	I2C_out(Powercontrol);
	I2C_out(Followercontrol);
	I2C_out(Displayon);
	I2C_out(Entrymode);
	I2C_out(Clear);
	I2C_Stop();
}
/*****************************************************/


// This program 
//   (1) prints to the UART and it reads from the UART
//   (2) it "prints" what it reads from the UART to portb (where LEDs are connected)
//   (3) it uses two timers to interrupt at different rates and drive 2 LEDs (on portb)
void main (void)
{
	// ensure the two lines are set for input (we are a slave)
	TRISBbits.TRISB4=0;
	TRISBbits.TRISB5=0;
	//OpenI2C
	SSP1STAT = 0x0; //Slew off
	SSP1CON1 = 0x0;
	SSP1CON2 = 0x0;
	SSP1CON1 |= 0x14;  // enable master
	SSP1ADD = 29;
	SSP1STAT |= SLEW_OFF;	
	IdleI2C1();
	init_LCD();

	while(1)
	{

		Show("ECE4534 is fun!");
	}
}