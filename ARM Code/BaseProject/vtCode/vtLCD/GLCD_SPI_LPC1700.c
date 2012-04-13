/******************************************************************************/
/* GLCD_SPI_LPC1700.c: LPC1700 low level Graphic LCD (240x320 pixels) driven  */
/*                     with SPI functions                                     */
/******************************************************************************/
/* This file is part of the uVision/ARM development tools.                    */
/* Copyright (c) 2005-2010 Keil Software. All rights reserved.                */
/* This software may only be used under the terms of a valid, current,        */
/* end user licence from KEIL for a compatible version of KEIL software       */
/* development tools. Nothing else gives you the right to use this software.  */
/******************************************************************************/


#include <lpc17xx.h>
#include "GLCD.h"
#include "Font_6x8_h.h"
#include "Font_16x24_h.h"
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "semphr.h"

// SPI (and supporting) include files from NXP
#include "lpc17xx_ssp.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_clkpwr.h"

// Include the VT SPI interrupt code
#include "vtSSP.h"

/************************** Orientation  configuration ************************/

#define HORIZONTAL  1                   /* If vertical = 0, if horizontal = 1 */

/*********************** Hardware specific configuration **********************/

/* SPI Interface: SPI3
   
   PINS: 
   - CS     = P0.6 (GPIO pin)
   - RS     = GND
   - WR/SCK = P0.7 (SCK1)
   - RD     = GND
   - SDO    = P0.8 (MISO1)
   - SDI    = P0.9 (MOSI1)                                                    */

#define PIN_CS      (1 << 6)

/* SPI_SR - bit definitions                                                   */
#define TFE         0x01
#define RNE         0x04
#define BSY         0x10

/*------------------------- Speed dependant settings -------------------------*/

/* If processor works on high frequency delay has to be increased, it can be 
   increased by factor 2^N by this constant                                   */
// MTJ removed and replaced with a real RTOS delay #define DELAY_2N    18

/*---------------------- Graphic LCD size definitions ------------------------*/

#if (HORIZONTAL == 1)
#define WIDTH       320                 /* Screen Width (in pixels)           */
#define HEIGHT      240                 /* Screen Hight (in pixels)           */
#else
#define WIDTH       240                 /* Screen Width (in pixels)           */
#define HEIGHT      320                 /* Screen Hight (in pixels)           */
#endif
#define BPP         16                  /* Bits per pixel                     */
#define BYPP        ((BPP+7)/8)         /* Bytes per pixel                    */

/*--------------- Graphic LCD interface hardware definitions -----------------*/

/* Pin CS setting to 0 or 1                                                   */
//MTJ change 
//#define LCD_CS(x)   ((x) ? (LPC_GPIO0->FIOSET = PIN_CS) : (LPC_GPIO0->FIOCLR = PIN_CS));
//#define LCD_CS(x)

#define SPI_START   (0x70)              /* Start byte for SPI transfer        */
#define SPI_RD      (0x01)              /* WR bit 1 within start              */
#define SPI_WR      (0x00)              /* WR bit 0 within start              */
#define SPI_DATA    (0x02)              /* RS bit 1 within start byte         */
#define SPI_INDEX   (0x00)              /* RS bit 0 within start byte         */
 
/*---------------------------- Global variables ------------------------------*/

/******************************************************************************/
static volatile unsigned short TextColor = Black, BackColor = White;


/************************ Local auxiliary functions ***************************/

/*******************************************************************************
* Delay in while loop cycles                                                   *
*   Parameter:    cnt:    number of while cycles to delay                      *
*   Return:                                                                    *
*******************************************************************************/

static void delay (int cnt) {

  /* MTJ modification for FreeRTOS 
  	CNT is actually in 10ms increments
	Instead of a busy loop (which is bad and unpredictable depending on compiler settings)
	I put in a FreeRTOS delay call 	   */
  /*
  cnt <<= DELAY_2N;
  while (cnt--);
  */
  vTaskDelay(cnt*10/portTICK_RATE_MS);
}
static unsigned char delay_val;
void LCD_CS(unsigned char val)
{
	if (val == 0) {
		// Make sure that the unit is idle
		while (SSP_GetStatus(LPC_SSP1,SSP_STAT_BUSY)==SET);
		LPC_GPIO0->FIOCLR = PIN_CS;
	} else {
		// Delay before & after	 and make sure that the unit is idle
		while (SSP_GetStatus(LPC_SSP1,SSP_STAT_BUSY)==SET);
		delay_val = 10;
		while (delay_val--);
		LPC_GPIO0->FIOSET = PIN_CS;
		delay_val = 10;
		while (delay_val--); 
	}
		
}
#if 0
/*******************************************************************************
* Send 1 byte over the serial communication                                    *
*   Parameter:    byte:   byte to be sent                                      *
*   Return:               byte read while sending                              *
*******************************************************************************/

static unsigned char spi_send (unsigned char byte) {
  unsigned char retval;
  SSP_DATA_SETUP_Type dataCfg;
  
  dataCfg.tx_data = &byte;
  dataCfg.rx_data = &retval;
  dataCfg.length = 1;

  SSP_ReadWrite(LPC_SSP1,&dataCfg,SSP_TRANSFER_POLLING);
  return(retval);
#if 0
  LPC_SSP1->DR = byte;
  while (!(LPC_SSP1->SR & RNE));        /* Wait for send to finish            */
  return (LPC_SSP1->DR); 
#endif
#if 0
  SSP_SendData(LPC_SSP1,byte);
  while (!(SSP_GetStatus(LPC_SSP1,SSP_STAT_RXFIFO_NOTEMPTY)	== SET));
  return(SSP_ReceiveData(LPC_SSP1));  
#endif
}
#endif


/*******************************************************************************
* Write a command the LCD controller                                           *
*   Parameter:    cmd:    command to be written                                *
*   Return:                                                                    *
*******************************************************************************/

static void wr_cmd (unsigned char cmd) {

  LCD_CS(0);
  //spi_send(SPI_START | SPI_WR | SPI_INDEX);   /* Write : RS = 0, RW = 0       */
  //spi_send(0);
  //spi_send(cmd);
  SSP_DATA_SETUP_Type dataCfg;
  
  unsigned char tbuf[3];
  tbuf[0] = SPI_START | SPI_WR | SPI_INDEX;
  tbuf[1] = 0;
  tbuf[2] = cmd;
  dataCfg.tx_data = tbuf;
  dataCfg.rx_data = NULL;
  dataCfg.length = 3;
  SSP_ReadWrite(LPC_SSP1,&dataCfg,SSP_TRANSFER_POLLING);
  LCD_CS(1);
}


/*******************************************************************************
* Write data to the LCD controller                                             *
*   Parameter:    dat:    data to be written                                   *
*   Return:                                                                    *
*******************************************************************************/

static void wr_dat (unsigned short dat) {

  LCD_CS(0);
  //spi_send(SPI_START | SPI_WR | SPI_DATA);    /* Write : RS = 1, RW = 0       */
  //spi_send((dat >>   8));                     /* Write D8..D15                */
  //spi_send((dat & 0xFF));                     /* Write D0..D7                 */
  SSP_DATA_SETUP_Type dataCfg;
  
  unsigned char tbuf[3];
  tbuf[0] = SPI_START | SPI_WR | SPI_DATA;
  tbuf[1] = dat >> 8;
  tbuf[2] = dat & 0xFF;
  dataCfg.tx_data = tbuf;
  dataCfg.rx_data = NULL;
  dataCfg.length = 3;
  SSP_ReadWrite(LPC_SSP1,&dataCfg,SSP_TRANSFER_POLLING);
  LCD_CS(1);
}


/*******************************************************************************
* Start of data writing to the LCD controller                                  *
*   Parameter:                                                                 *
*   Return:                                                                    *
*******************************************************************************/

static void wr_dat_start (void) {
  unsigned char outData = SPI_START | SPI_WR | SPI_DATA;
  SSP_DATA_SETUP_Type dataCfg;
  LCD_CS(0);
  //spi_send(SPI_START | SPI_WR | SPI_DATA);    /* Write : RS = 1, RW = 0       */
  dataCfg.tx_data = &outData;
  dataCfg.rx_data = NULL;
  dataCfg.length = 1;
  SSP_ReadWrite(LPC_SSP1,&dataCfg,SSP_TRANSFER_POLLING);
}


/*******************************************************************************
* Stop of data writing to the LCD controller                                   *
*   Parameter:                                                                 *
*   Return:                                                                    *
*******************************************************************************/

static void wr_dat_stop (void) {

  LCD_CS(1);
}


/*******************************************************************************
* Data writing to the LCD controller                                           *
*   Parameter:    dat:    data to be written                                   *
*   Return:                                                                    *
*******************************************************************************/

static void wr_dat_only (unsigned short dat) {

  //spi_send((dat >>   8));                     /* Write D8..D15                */
  //spi_send((dat & 0xFF));                     /* Write D0..D7                 */
  SSP_DATA_SETUP_Type dataCfg;
  
  unsigned char tbuf[2];
  tbuf[0] = dat >> 8;
  tbuf[1] = dat & 0xFF;
  dataCfg.tx_data = tbuf;
  dataCfg.rx_data = NULL;
  dataCfg.length = 2;
  SSP_ReadWrite(LPC_SSP1,&dataCfg,SSP_TRANSFER_POLLING);
}


/*******************************************************************************
* Read data from the LCD controller                                            *
*   Parameter:                                                                 *
*   Return:               read data                                            *
*******************************************************************************/

static unsigned short rd_dat (void) {
#if 0
  unsigned short val = 0;

  LCD_CS(0);
  spi_send(SPI_START | SPI_RD | SPI_DATA);    /* Read: RS = 1, RW = 1         */
  spi_send(0);                                /* Dummy read 1                 */
  val   = spi_send(0);                        /* Read D8..D15                 */
  val <<= 8;
  val  |= spi_send(0);                        /* Read D0..D7                  */
  LCD_CS(1);
#endif
  SSP_DATA_SETUP_Type dataCfg; 
  unsigned char tbuf[4];
  unsigned char rbuf[4];
  unsigned short val;
  LCD_CS(0);
  tbuf[0] = SPI_START | SPI_RD | SPI_DATA;
  tbuf[1] = 0x0;
  tbuf[2] = 0x0;
  tbuf[3] = 0x0;
  dataCfg.tx_data = tbuf;
  dataCfg.rx_data = rbuf;
  dataCfg.length = 4;
  SSP_ReadWrite(LPC_SSP1,&dataCfg,SSP_TRANSFER_POLLING);
  val = rbuf[2];
  val = val << 8;
  val = val + rbuf[3];
  LCD_CS(1);
  return (val);
}


/*******************************************************************************
* Write a value to the to LCD register                                         *
*   Parameter:    reg:    register to be written                               *
*                 val:    value to write to the register                       *
*******************************************************************************/

static void wr_reg (unsigned char reg, unsigned short val) {

  wr_cmd(reg);
  wr_dat(val);
}


/*******************************************************************************
* Read from the LCD register                                                   *
*   Parameter:    reg:    register to be read                                  *
*   Return:               value read from the register                         *
*******************************************************************************/

static unsigned short rd_reg (unsigned char reg) {

  wr_cmd(reg);
  return(rd_dat());
}


/************************ Exported functions **********************************/

/*******************************************************************************
* Initialize the Graphic LCD controller                                        *
*   Parameter:                                                                 *
*   Return:                                                                    *
*******************************************************************************/

void GLCD_Init (void) { 
  unsigned short driverCode;
  PINSEL_CFG_Type PinCfg;
  SSP_CFG_Type SSP_ConfigStruct;
  /* Enable clock for SSP1, clock = CCLK / 2                                  */
  //LPC_SC->PCONP       |= 0x00000400;
  //LPC_SC->PCLKSEL0    |= 0x00200000;
  CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_SSP1,2);   

  /* Configure the LCD Control pins                                           */
  LPC_PINCON->PINSEL9 &= 0xF0FFFFFF;
  LPC_GPIO4->FIODIR   |= 0x30000000;
  LPC_GPIO4->FIOSET    = 0x20000000;

  /* SSEL1 is GPIO output set to high                                         */
  /* LPC_GPIO0->FIODIR   |= 0x00000040;
  LPC_GPIO0->FIOSET    = 0x00000040;   */
  /*LPC_PINCON->PINSEL0 &= 0xFFF03FFF;
  LPC_PINCON->PINSEL0 |= 0x000A8000; */

  /* Enable SPI in Master Mode, CPOL=1, CPHA=1                                */
  /* Max. 12.5 MBit used for Data Transfer @ 100MHz                           */
  /*LPC_SSP1->CR0        = 0x01C7;
  LPC_SSP1->CPSR       = 0x02;
  LPC_SSP1->CR1        = 0x02;	*/
  // MTJ: Here is the right way to initialize this unit
  // configure the pins for SSP1
  // P0.6 -- SSEL1
  // P0.7 -- SCK1
  // P0.8 -- MISO1
  // P0.9 -- MOSI1
  // Set P0.6 (as GPIO) to have an output of '1' (idle CS)
  GPIO_SetDir(0,0x00000040,1);
  GPIO_SetValue(0,0x00000040);
  PinCfg.Funcnum = 0;
  PinCfg.OpenDrain = 0;
  PinCfg.Pinmode = 0;
  PinCfg.Portnum = 0;
  PinCfg.Pinnum = 6;
  PINSEL_ConfigPin(&PinCfg);
  PinCfg.Funcnum = 2;
  PinCfg.Pinnum = 7;
  PINSEL_ConfigPin(&PinCfg);
  PinCfg.Pinnum = 8;
  PINSEL_ConfigPin(&PinCfg);
  PinCfg.Pinnum = 9;
  PINSEL_ConfigPin(&PinCfg);
  // initialize the configuration struction with default values
  SSP_ConfigStructInit(&SSP_ConfigStruct);
  // then change the values we care about
  SSP_ConfigStruct.ClockRate = 0xBEBC20; // 12.5MHz
  SSP_ConfigStruct.CPOL = SSP_CPOL_LO;
  SSP_ConfigStruct.CPHA = SSP_CPHA_SECOND;
  // initialize the SSP1 unit
  SSP_Init(LPC_SSP1,&SSP_ConfigStruct);
  LPC_SSP1->IMSC = 0;
  // now turn it on
  SSP_Cmd(LPC_SSP1,ENABLE);
  // End of new initialization

  delay(5);                             /* Delay 50 ms                        */
  driverCode = rd_reg(0x00);

  /* Start Initial Sequence --------------------------------------------------*/
  wr_reg(0x01, 0x0100);                 /* Set SS bit                         */
  wr_reg(0x02, 0x0700);                 /* Set 1 line inversion               */
  wr_reg(0x04, 0x0000);                 /* Resize register                    */
  wr_reg(0x08, 0x0207);                 /* 2 lines front, 7 back porch        */
  wr_reg(0x09, 0x0000);                 /* Set non-disp area refresh cyc ISC  */
  wr_reg(0x0A, 0x0000);                 /* FMARK function                     */
  wr_reg(0x0C, 0x0000);                 /* RGB interface setting              */
  wr_reg(0x0D, 0x0000);                 /* Frame marker Position              */
  wr_reg(0x0F, 0x0000);                 /* RGB interface polarity             */

  /* Power On sequence -------------------------------------------------------*/
  wr_reg(0x10, 0x0000);                 /* Reset Power Control 1              */
  wr_reg(0x11, 0x0000);                 /* Reset Power Control 2              */
  wr_reg(0x12, 0x0000);                 /* Reset Power Control 3              */
  wr_reg(0x13, 0x0000);                 /* Reset Power Control 4              */
  delay(20);                            /* Discharge cap power voltage (200ms)*/
  wr_reg(0x10, 0x12B0);                 /* SAP, BT[3:0], AP, DSTB, SLP, STB   */
  wr_reg(0x11, 0x0007);                 /* DC1[2:0], DC0[2:0], VC[2:0]        */
  delay(5);                             /* Delay 50 ms                        */
  wr_reg(0x12, 0x01BD);                 /* VREG1OUT voltage                   */
  delay(5);                             /* Delay 50 ms                        */
  wr_reg(0x13, 0x1400);                 /* VDV[4:0] for VCOM amplitude        */
  wr_reg(0x29, 0x000E);                 /* VCM[4:0] for VCOMH                 */
  delay(5);                             /* Delay 50 ms                        */
  wr_reg(0x20, 0x0000);                 /* GRAM horizontal Address            */
  wr_reg(0x21, 0x0000);                 /* GRAM Vertical Address              */

  /* Adjust the Gamma Curve --------------------------------------------------*/
  if (driverCode == 0x5408) {           /* LCD with SPFD5408 LCD Controller   */
    wr_reg(0x30, 0x0B0D);
    wr_reg(0x31, 0x1923);
    wr_reg(0x32, 0x1C26);
    wr_reg(0x33, 0x261C);
    wr_reg(0x34, 0x2419);
    wr_reg(0x35, 0x0D0B);
    wr_reg(0x36, 0x1006);
    wr_reg(0x37, 0x0610);
    wr_reg(0x38, 0x0706);
    wr_reg(0x39, 0x0304);
    wr_reg(0x3A, 0x0E05);
    wr_reg(0x3B, 0x0E01);
    wr_reg(0x3C, 0x010E);
    wr_reg(0x3D, 0x050E);
    wr_reg(0x3E, 0x0403);
    wr_reg(0x3F, 0x0607);
  }
  else {                                /* LCD with other LCD Controller      */
    wr_reg(0x30, 0x0006);
    wr_reg(0x31, 0x0101);
    wr_reg(0x32, 0x0003);
    wr_reg(0x35, 0x0106);
    wr_reg(0x36, 0x0B02);
    wr_reg(0x37, 0x0302);
    wr_reg(0x38, 0x0707);
    wr_reg(0x39, 0x0007);
    wr_reg(0x3C, 0x0600);
    wr_reg(0x3D, 0x020B);
  }

  /* Set GRAM area -----------------------------------------------------------*/
  wr_reg(0x50, 0x0000);                 /* Horizontal GRAM Start Address      */
  wr_reg(0x51, (HEIGHT-1));             /* Horizontal GRAM End   Address      */
  wr_reg(0x52, 0x0000);                 /* Vertical   GRAM Start Address      */
  wr_reg(0x53, (WIDTH-1));              /* Vertical   GRAM End   Address      */
  if (driverCode == 0x5408)             /* LCD with SPFD5408 LCD Controller   */
    wr_reg(0x60, 0xA700);               /* Gate Scan Line                     */
  else                                  /* LCD with other LCD Controller      */
    wr_reg(0x60, 0x2700);               /* Gate Scan Line                     */
  wr_reg(0x61, 0x0001);                 /* NDL,VLE, REV                       */
  wr_reg(0x6A, 0x0000);                 /* Set scrolling line                 */

  /* Partial Display Control -------------------------------------------------*/
  wr_reg(0x80, 0x0000);
  wr_reg(0x81, 0x0000);
  wr_reg(0x82, 0x0000);
  wr_reg(0x83, 0x0000);
  wr_reg(0x84, 0x0000);
  wr_reg(0x85, 0x0000);

  /* Panel Control -----------------------------------------------------------*/
  wr_reg(0x90, 0x0010);
  wr_reg(0x92, 0x0000);
  wr_reg(0x93, 0x0003);
  wr_reg(0x95, 0x0110);
  wr_reg(0x97, 0x0000);
  wr_reg(0x98, 0x0000);

  /* Set GRAM write direction
     I/D=11 (Horizontal : increment, Vertical : increment)                    */

#if (HORIZONTAL == 1)
  /* AM=1   (address is updated in vertical writing direction)                */
  wr_reg(0x03, 0x1038);
#else 
  /* AM=0   (address is updated in horizontal writing direction)              */
  wr_reg(0x03, 0x1030);
#endif

  wr_reg(0x07, 0x0137);                 /* 262K color and display ON          */
  LPC_GPIO4->FIOSET = 0x10000000;	  // Turn on the backlight

  // Initialize the interrupt driver for bulk SPI transfer on SSP1
  if (vtSSPIsrInit(1) != vtSSPInitSuccess) {
  	VT_HANDLE_FATAL_ERROR(0);
  }
}

/*******************************************************************************
* Initialize the Graphic LCD controller and touchscreen                        *
*   Parameter:                                                                 *
*   Return:                                                                    *
*******************************************************************************/

void GLCD_TSInit (void) { 
  unsigned short driverCode;
  PINSEL_CFG_Type PinCfg;
  SSP_CFG_Type SSP_ConfigStruct;
  /* Enable clock for SSP1, clock = CCLK / 2                                  */
  //LPC_SC->PCONP       |= 0x00000400;
  //LPC_SC->PCLKSEL0    |= 0x00200000;
  CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_SSP1,2);   

  /* Configure the LCD Control pins                                           */
  LPC_PINCON->PINSEL9 &= 0xF0FFFFFF;
  LPC_GPIO4->FIODIR   |= 0x30000000;
  LPC_GPIO4->FIOSET    = 0x20000000;

  /* SSEL1 is GPIO output set to high                                         */
  /* LPC_GPIO0->FIODIR   |= 0x00000040;
  LPC_GPIO0->FIOSET    = 0x00000040;   */
  /*LPC_PINCON->PINSEL0 &= 0xFFF03FFF;
  LPC_PINCON->PINSEL0 |= 0x000A8000; */

  /* Enable SPI in Master Mode, CPOL=1, CPHA=1                                */
  /* Max. 12.5 MBit used for Data Transfer @ 100MHz                           */
  /*LPC_SSP1->CR0        = 0x01C7;
  LPC_SSP1->CPSR       = 0x02;
  LPC_SSP1->CR1        = 0x02;	*/
  // MTJ: Here is the right way to initialize this unit
  // configure the pins for SSP1
  // P0.6 -- SSEL1
  // P0.7 -- SCK1
  // P0.8 -- MISO1
  // P0.9 -- MOSI1
  // Set P0.6 (as GPIO) to have an output of '1' (idle CS)
  GPIO_SetDir(0,0x00000040,1);
  GPIO_SetValue(0,0x00000040);
  PinCfg.Funcnum = 0;
  PinCfg.OpenDrain = 0;
  PinCfg.Pinmode = 0;
  PinCfg.Portnum = 0;
  PinCfg.Pinnum = 6;
  PINSEL_ConfigPin(&PinCfg);
  PinCfg.Funcnum = 2;
  PinCfg.Pinnum = 7;
  PINSEL_ConfigPin(&PinCfg);
  PinCfg.Pinnum = 8;
  PINSEL_ConfigPin(&PinCfg);
  PinCfg.Pinnum = 9;
  PINSEL_ConfigPin(&PinCfg);
  // initialize the configuration struction with default values
  SSP_ConfigStructInit(&SSP_ConfigStruct);
  // then change the values we care about
  SSP_ConfigStruct.ClockRate = 0xBEBC20; // 12.5MHz
  SSP_ConfigStruct.CPOL = SSP_CPOL_LO;
  SSP_ConfigStruct.CPHA = SSP_CPHA_SECOND;
  // initialize the SSP1 unit
  SSP_Init(LPC_SSP1,&SSP_ConfigStruct);
  LPC_SSP1->IMSC = 0;
  // now turn it on
  SSP_Cmd(LPC_SSP1,ENABLE);
  // End of new initialization

  delay(5);                             /* Delay 50 ms                        */
  driverCode = rd_reg(0x00);

  /* Start Initial Sequence --------------------------------------------------*/
  wr_reg(0x01, 0x0100);                 /* Set SS bit                         */
  wr_reg(0x02, 0x0700);                 /* Set 1 line inversion               */
  wr_reg(0x04, 0x0000);                 /* Resize register                    */
  wr_reg(0x08, 0x0207);                 /* 2 lines front, 7 back porch        */
  wr_reg(0x09, 0x0000);                 /* Set non-disp area refresh cyc ISC  */
  wr_reg(0x0A, 0x0000);                 /* FMARK function                     */
  wr_reg(0x0C, 0x0000);                 /* RGB interface setting              */
  wr_reg(0x0D, 0x0000);                 /* Frame marker Position              */
  wr_reg(0x0F, 0x0000);                 /* RGB interface polarity             */

  /* Power On sequence -------------------------------------------------------*/
  wr_reg(0x10, 0x0000);                 /* Reset Power Control 1              */
  wr_reg(0x11, 0x0000);                 /* Reset Power Control 2              */
  wr_reg(0x12, 0x0000);                 /* Reset Power Control 3              */
  wr_reg(0x13, 0x0000);                 /* Reset Power Control 4              */
  delay(20);                            /* Discharge cap power voltage (200ms)*/
  wr_reg(0x10, 0x12B0);                 /* SAP, BT[3:0], AP, DSTB, SLP, STB   */
  wr_reg(0x11, 0x0007);                 /* DC1[2:0], DC0[2:0], VC[2:0]        */
  delay(5);                             /* Delay 50 ms                        */
  wr_reg(0x12, 0x01BD);                 /* VREG1OUT voltage                   */
  delay(5);                             /* Delay 50 ms                        */
  wr_reg(0x13, 0x1400);                 /* VDV[4:0] for VCOM amplitude        */
  wr_reg(0x29, 0x000E);                 /* VCM[4:0] for VCOMH                 */
  delay(5);                             /* Delay 50 ms                        */
  wr_reg(0x20, 0x0000);                 /* GRAM horizontal Address            */
  wr_reg(0x21, 0x0000);                 /* GRAM Vertical Address              */

  /* Adjust the Gamma Curve --------------------------------------------------*/
  if (driverCode == 0x5408) {           /* LCD with SPFD5408 LCD Controller   */
    wr_reg(0x30, 0x0B0D);
    wr_reg(0x31, 0x1923);
    wr_reg(0x32, 0x1C26);
    wr_reg(0x33, 0x261C);
    wr_reg(0x34, 0x2419);
    wr_reg(0x35, 0x0D0B);
    wr_reg(0x36, 0x1006);
    wr_reg(0x37, 0x0610);
    wr_reg(0x38, 0x0706);
    wr_reg(0x39, 0x0304);
    wr_reg(0x3A, 0x0E05);
    wr_reg(0x3B, 0x0E01);
    wr_reg(0x3C, 0x010E);
    wr_reg(0x3D, 0x050E);
    wr_reg(0x3E, 0x0403);
    wr_reg(0x3F, 0x0607);
  }
  else {                                /* LCD with other LCD Controller      */
    wr_reg(0x30, 0x0006);
    wr_reg(0x31, 0x0101);
    wr_reg(0x32, 0x0003);
    wr_reg(0x35, 0x0106);
    wr_reg(0x36, 0x0B02);
    wr_reg(0x37, 0x0302);
    wr_reg(0x38, 0x0707);
    wr_reg(0x39, 0x0007);
    wr_reg(0x3C, 0x0600);
    wr_reg(0x3D, 0x020B);
  }

  /* Set GRAM area -----------------------------------------------------------*/
  wr_reg(0x50, 0x0000);                 /* Horizontal GRAM Start Address      */
  wr_reg(0x51, (HEIGHT-1));             /* Horizontal GRAM End   Address      */
  wr_reg(0x52, 0x0000);                 /* Vertical   GRAM Start Address      */
  wr_reg(0x53, (WIDTH-1));              /* Vertical   GRAM End   Address      */
  if (driverCode == 0x5408)             /* LCD with SPFD5408 LCD Controller   */
    wr_reg(0x60, 0xA700);               /* Gate Scan Line                     */
  else                                  /* LCD with other LCD Controller      */
    wr_reg(0x60, 0x2700);               /* Gate Scan Line                     */
  wr_reg(0x61, 0x0001);                 /* NDL,VLE, REV                       */
  wr_reg(0x6A, 0x0000);                 /* Set scrolling line                 */

  /* Partial Display Control -------------------------------------------------*/
  wr_reg(0x80, 0x0000);
  wr_reg(0x81, 0x0000);
  wr_reg(0x82, 0x0000);
  wr_reg(0x83, 0x0000);
  wr_reg(0x84, 0x0000);
  wr_reg(0x85, 0x0000);

  /* Panel Control -----------------------------------------------------------*/
  wr_reg(0x90, 0x0010);
  wr_reg(0x92, 0x0000);
  wr_reg(0x93, 0x0003);
  wr_reg(0x95, 0x0110);
  wr_reg(0x97, 0x0000);
  wr_reg(0x98, 0x0000);

  /* Set GRAM write direction
     I/D=11 (Horizontal : increment, Vertical : increment)                    */

#if (HORIZONTAL == 1)
  /* AM=1   (address is updated in vertical writing direction)                */
  wr_reg(0x03, 0x1038);
#else 
  /* AM=0   (address is updated in horizontal writing direction)              */
  wr_reg(0x03, 0x1030);
#endif

  wr_reg(0x07, 0x0137);                 /* 262K color and display ON          */
  LPC_GPIO4->FIOSET = 0x10000000;	  // Turn on the backlight

  // Initialize the interrupt driver for bulk SPI transfer on SSP1
  if (vtSSPIsrInit(1) != vtSSPInitSuccess) {
  	VT_HANDLE_FATAL_ERROR(0);
  }
}

/*******************************************************************************
* Set draw window region                                                       *
*   Parameter:      x:        horizontal position                              *
*                   y:        vertical position                                *
*                   w:        window width in pixel                            *
*                   h:        window height in pixels                          *
*   Return:                                                                    *
*******************************************************************************/

void GLCD_SetWindow (unsigned int x, unsigned int y, unsigned int w, unsigned int h) {

  wr_reg(0x50, x);                      /* Horizontal GRAM Start Address      */
  wr_reg(0x51, x+w-1);                  /* Horizontal GRAM End   Address (-1) */
  wr_reg(0x52, y);                      /* Vertical   GRAM Start Address      */
  wr_reg(0x53, y+h-1);                  /* Vertical   GRAM End   Address (-1) */

  wr_reg(0x20, x);
  wr_reg(0x21, y);
}


/*******************************************************************************
* Set draw window region to whole screen                                       *
*   Parameter:                                                                 *
*   Return:                                                                    *
*******************************************************************************/

void GLCD_WindowMax (void) {

#if (HORIZONTAL == 1)
  GLCD_SetWindow (0, 0, HEIGHT, WIDTH);
#else
  GLCD_SetWindow (0, 0, WIDTH,  HEIGHT);
#endif
}


/*******************************************************************************
* Draw a pixel in foreground color                                             *
*   Parameter:      x:        horizontal position                              *
*                   y:        vertical position                                *
*   Return:                                                                    *
*******************************************************************************/

void GLCD_PutPixel (unsigned int x, unsigned int y) {

#if (HORIZONTAL == 1)
  wr_reg(0x20, y);
  wr_reg(0x21, WIDTH-1-x);
#else
  wr_reg(0x20, x);
  wr_reg(0x21, y);
#endif
  wr_cmd(0x22);
  wr_dat(TextColor);
}


/*******************************************************************************
* Set foreground color                                                         *
*   Parameter:      color:    foreground color                                 *
*   Return:                                                                    *
*******************************************************************************/

void GLCD_SetTextColor (unsigned short color) {

  TextColor = color;
}


/*******************************************************************************
* Set background color                                                         *
*   Parameter:      color:    background color                                 *
*   Return:                                                                    *
*******************************************************************************/

void GLCD_SetBackColor (unsigned short color) {

  BackColor = color;
}



/*******************************************************************************
* Clear display                                                                *
*   Parameter:      color:    display clearing color                           *
*   Return:                                                                    *
*******************************************************************************/
unsigned short colorBuf[WIDTH];
void GLCD_Clear (unsigned short color) {
  unsigned int i;
  vtSSPIsrData dataCfg;  
  unsigned char *tptr = (unsigned char *) colorBuf;
  
  dataCfg.tx_data = colorBuf;
  dataCfg.length = WIDTH*sizeof(unsigned short);

  tptr[0] = color >> 8;
  tptr[1] = color & 0xFF;
  for (i=1;i<WIDTH;i++) colorBuf[i] = colorBuf[0];
  GLCD_WindowMax();
  wr_cmd(0x22);
  wr_dat_start();
  for (i=0;i<HEIGHT;i++) {
	vtSSPStartOperation(&dataCfg);
	if (vtSSPWaitComplete(portMAX_DELAY) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(0);
	}
  }
  wr_dat_stop();
}

/*******************************************************************************
* Clear display                                                                *
*   Parameter:      color:    display clearing color                           *
*   Return:                                                                    *
*******************************************************************************/

void GLCD_ClearArea (unsigned short color, unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
  GLCD_WindowMax;
  unsigned short colorBuf2[WIDTH];
  unsigned int i;
  vtSSPIsrData dataCfg;  
  unsigned char *tptr = (unsigned char *) colorBuf;
  
  dataCfg.tx_data = colorBuf;
  dataCfg.length = WIDTH*sizeof(unsigned short);

  tptr[0] = color >> 8;
  tptr[1] = color & 0xFF;
  for (i=1;i<WIDTH;i++) colorBuf[i] = colorBuf[0];
  GLCD_SetWindow(x, y, w, h);
  wr_cmd(0x22);
  wr_dat_start();
  for (i=0;i<HEIGHT;i++) {
	vtSSPStartOperation(&dataCfg);
	if (vtSSPWaitComplete(portMAX_DELAY) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(0);
	}
  }
  wr_dat_stop();
}


/*******************************************************************************
* Draw character on given position                                             *
*   Parameter:      x:        horizontal position                              *
*                   y:        vertical position                                *
*                   cw:       character width in pixel                         *
*                   ch:       character height in pixels                       *
*                   c:        pointer to character bitmap                      *
*   Return:                                                                    *
*******************************************************************************/

void GLCD_DrawChar_U8 (unsigned int x, unsigned int y, unsigned int cw, unsigned int ch, unsigned char *c) {
  int idx = 0, i, j;

#if (HORIZONTAL == 1)
  x = WIDTH-x-cw;
  GLCD_SetWindow(y, x, ch, cw);
#else
  GLCD_SetWindow(x, y, cw, ch);
#endif
  wr_cmd(0x22);
  wr_dat_start();
  for (j = 0; j < ch; j++) {
#if (HORIZONTAL == 1)
    for (i = cw-1; i >= 0; i--) {
#else
    for (i = 0; i <= cw-1; i++) {
#endif
      if((c[idx] & (1 << i)) == 0x00) {
        wr_dat_only(BackColor);
      } else {
        wr_dat_only(TextColor);
      }
    }
    c++;
  }
  wr_dat_stop();
}


/*******************************************************************************
* Draw character on given position                                             *
*   Parameter:      x:        horizontal position                              *
*                   y:        vertical position                                *
*                   cw:       character width in pixel                         *
*                   ch:       character height in pixels                       *
*                   c:        pointer to character bitmap                      *
*   Return:                                                                    *
*******************************************************************************/

void GLCD_DrawChar_U16 (unsigned int x, unsigned int y, unsigned int cw, unsigned int ch, unsigned short *c) {
  int idx = 0, i, j;
  int cnt;
  unsigned short revBackColor;
  unsigned short revTextColor;
  vtSSPIsrData dataCfg;

  revBackColor = BackColor >> 8;
  revBackColor += BackColor << 8;
  revTextColor = TextColor >> 8;
  revTextColor += TextColor << 8;

#if (HORIZONTAL == 1)
  x = WIDTH-x-cw;
  if (x < 0) {
    // writing past the end of the line -- ignore it
  	return;
  }
  if (y+ch > HEIGHT) {
    // writing beyond the bottom of the screen -- ignore it
  	return;
  } 
  GLCD_SetWindow(y, x, ch, cw);
#else
  if (x+cw > WIDTH)) {
    // writing past the end of the line -- ignore it
  	return;
  }
  if (y+ch > HEIGHT) {
    // writing beyond the bottom of the screen -- ignore it
  	return;
  }
  GLCD_SetWindow(x, y, cw, ch);
#endif
  dataCfg.tx_data = colorBuf;
  dataCfg.length = cw*sizeof(unsigned short);

  wr_cmd(0x22);
  wr_dat_start();
  for (j = 0; j < ch; j++) {
    cnt = 0;
#if (HORIZONTAL == 1)
    for (i = cw-1; i >= 0; i--) {
#else
    for (i = 0; i <= cw-1; i++) {
#endif
      if((c[idx] & (1 << i)) == 0x00) {
        colorBuf[cnt] = revBackColor; cnt++; //wr_dat_only(BackColor);
      } else {
        colorBuf[cnt] = revTextColor; cnt++; //wr_dat_only(TextColor);
      }
    }
	vtSSPStartOperation(&dataCfg);
	if (vtSSPWaitComplete(portMAX_DELAY) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(0);
	}
    c++;
  }
  wr_dat_stop();
}


/*******************************************************************************
* Disply character on given line                                               *
*   Parameter:      ln:       line number                                      *
*                   col:      column number                                    *
*                   fi:       font index (0 = 6x8, 1 = 16x24)                  *
*                   c:        ascii character                                  *
*   Return:                                                                    *
*******************************************************************************/

void GLCD_DisplayChar (unsigned int ln, unsigned int col, unsigned char fi, unsigned char c) {

  c -= 32;
  switch (fi) {
    case 0:  /* Font 6 x 8 */
      GLCD_DrawChar_U8 (col *  6, ln *  8,  6,  8, (unsigned char  *)&Font_6x8_h  [c * 8]);
      break;
    case 1:  /* Font 16 x 24 */
      GLCD_DrawChar_U16(col * 16, ln * 24, 16, 24, (unsigned short *)&Font_16x24_h[c * 24]);
      break;
  }
}


/*******************************************************************************
* Disply string on given line                                                  *
*   Parameter:      ln:       line number                                      *
*                   col:      column number                                    *
*                   fi:       font index (0 = 6x8, 1 = 16x24)                  *
*                   s:        pointer to string                                *
*   Return:                                                                    *
*******************************************************************************/

void GLCD_DisplayString (unsigned int ln, unsigned int col, unsigned char fi, unsigned char *s) {

  GLCD_WindowMax();
  while (*s) {
    GLCD_DisplayChar(ln, col++, fi, *s++);
  }
}


/*******************************************************************************
* Clear given line                                                             *
*   Parameter:      ln:       line number                                      *
*                   fi:       font index (0 = 6x8, 1 = 16x24)                  *
*   Return:                                                                    *
*******************************************************************************/

void GLCD_ClearLn (unsigned int ln, unsigned char fi) {
  int i;
  unsigned int cHeight, pixHeight;
  unsigned char *tptr = (unsigned char *) colorBuf;
  vtSSPIsrData dataCfg;

  switch(fi)  {
  	case 0: {
	  cHeight = 8;
	};
	break;
	case 1: {
	  cHeight = 24;
	};
	break;
	default: {
	  return;
	}
  }
  pixHeight = ln * cHeight;
  if (pixHeight + cHeight > HEIGHT) {
    // The specified line is out of bounds
    return;
  }
#if (HORIZONTAL == 1)
  GLCD_SetWindow(pixHeight, 0, cHeight, WIDTH);
#else
  GLCD_SetWindow(0, pixHeight, WIDTH, cHeight);
#endif
  dataCfg.tx_data = colorBuf;
  dataCfg.length = WIDTH*sizeof(unsigned short);

  tptr[0] = BackColor >> 8;
  tptr[1] = BackColor & 0xFF;
  for (i=1;i<WIDTH;i++) colorBuf[i] = colorBuf[0];
  wr_cmd(0x22);
  wr_dat_start();
  for (i=0;i<cHeight;i++) {
	vtSSPStartOperation(&dataCfg);
	if (vtSSPWaitComplete(portMAX_DELAY) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(0);
	}
  }
  wr_dat_stop();

#if 0
  unsigned char i;
  unsigned char buf[60];
  int len;

  switch (fi) {
  	case 0:
		len = (WIDTH+5)/6

  GLCD_WindowMax();
  switch (fi) {
    case 0:  /* Font 6 x 8 */
      for (i = 0; i < (WIDTH+5)/6; i++)
        buf[i] = ' ';
      buf[i+1] = 0;
      break;
    case 1:  /* Font 16 x 24 */
      for (i = 0; i < (WIDTH+15)/16; i++)
        buf[i] = ' ';
      buf[i+1] = 0;
      break;
  }
  GLCD_DisplayString (ln, 0, fi, buf);
#endif
}

/*******************************************************************************
* Draw bargraph                                                                *
*   Parameter:      x:        horizontal position                              *
*                   y:        vertical position                                *
*                   w:        maximum width of bargraph (in pixels)            *
*                   val:      value of active bargraph (in 1/1024)             *
*   Return:                                                                    *
*******************************************************************************/

void GLCD_Bargraph (unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int val) {
  int i,j;

  val = (val * w) >> 10;                /* Scale value                        */
#if (HORIZONTAL == 1)
  x = WIDTH-x-w;
  GLCD_SetWindow(y, x, h, w);
#else
  GLCD_SetWindow(x, y, w, h);
#endif
  wr_cmd(0x22);
  wr_dat_start();
  for (i = 0; i < h; i++) {
#if (HORIZONTAL == 1)
    for (j = w-1; j >= 0; j--) {
#else
    for (j = 0; j <= w-1; j++) {
#endif
      if(j >= val) {
        wr_dat_only(BackColor);
      } else {
        wr_dat_only(TextColor);
      }
    }
  }
  wr_dat_stop();
}


/*******************************************************************************
* Display graphical bitmap image at position x horizontally and y vertically   *
* (This function is optimized for 16 bits per pixel format, it has to be       *
*  adapted for any other bits per pixel format)                                *
*   Parameter:      x:        horizontal position                              *
*                   y:        vertical position                                *
*                   w:        width of bitmap                                  *
*                   h:        height of bitmap                                 *
*                   bitmap:   address at which the bitmap data resides         *
*   Return:                                                                    *
*******************************************************************************/

void GLCD_Bitmap (unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned char *bitmap) {
  unsigned int    i, j;
  unsigned short *bitmap_ptr = (unsigned short *)bitmap;

#if (HORIZONTAL == 1)
  x = WIDTH-x-w;
  GLCD_SetWindow(y, x, h, w);
#else
  GLCD_SetWindow(x, y, w, h);
#endif
  wr_cmd(0x22);
  wr_dat_start();
  for (j = 0; j < h; j++) {
#if (HORIZONTAL == 1)
    for (i = 0; i < w; i++) {
      wr_dat_only(*bitmap_ptr++);
    }
#else
    bitmap_ptr += w-1;
    for (i = 0; i < w; i++) {
      wr_dat_only(*bitmap_ptr--);
    }
    bitmap_ptr += w+1;
#endif
  }
  wr_dat_stop();
}


/*******************************************************************************
* Display graphical bmp file image at position x horizontally and y vertically *
* (This function is optimized for 16 bits per pixel format, it has to be       *
*  adapted for any other bits per pixel format)                                *
*   Parameter:      x:        horizontal position                              *
*                   y:        vertical position                                *
*                   w:        width of bitmap                                  *
*                   h:        height of bitmap                                 *
*                   bmp:      address at which the bmp data resides            *
*   Return:                                                                    *
*******************************************************************************/

void GLCD_Bmp (unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned char *bmp) {
  unsigned int    i, j;
  unsigned short *bitmap_ptr = (unsigned short *)bmp;
  vtSSPIsrData dataCfg;
  unsigned char *tbuf = (unsigned char *) colorBuf;
  unsigned int bufCnt;

#if (HORIZONTAL == 1)
  x = WIDTH-x-w;
  GLCD_SetWindow(y, x, h, w);
#else
  GLCD_SetWindow(x, y, w, h);
#endif
  dataCfg.tx_data = colorBuf;
  wr_cmd(0x22);
  wr_dat_start();
#if (HORIZONTAL == 1)
  bitmap_ptr += (h*w)-1;
  bufCnt = 0;
  for (j = 0; j < h; j++) {
    for (i = 0; i < w; i++) {
      //wr_dat_only(*bitmap_ptr--);
	  tbuf[bufCnt] = (*bitmap_ptr) >> 8; bufCnt++;
	  tbuf[bufCnt] = (*bitmap_ptr) & 0xFF; bufCnt++;
	  //colorBuf[bufCnt] = (*bitmap_ptr); bufCnt++;
	  if (bufCnt >= (WIDTH*2)-2) {
	    dataCfg.length = bufCnt;
		vtSSPStartOperation(&dataCfg);
		if (vtSSPWaitComplete(portMAX_DELAY) != pdPASS) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		bufCnt = 0;
	  }
	  bitmap_ptr--;
    }
  }
  if (bufCnt > 0) {
    dataCfg.length = bufCnt;
	vtSSPStartOperation(&dataCfg);
	if (vtSSPWaitComplete(portMAX_DELAY) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(0);
	}
  }
#else
  bitmap_ptr += ((h-1)*w);
  for (j = 0; j < h; j++) {
    for (i = 0; i < w; i++) {
      wr_dat_only(*bitmap_ptr++);
    }
    bitmap_ptr -= 2*w;
  }
#endif
  wr_dat_stop();
}


/*******************************************************************************
* Scroll content of the whole display for dy pixels vertically                 *
*   Parameter:      dy:       number of pixels for vertical scroll             *
*   Return:                                                                    *
*******************************************************************************/

void GLCD_ScrollVertical (unsigned int dy) {
#if (HORIZONTAL == 0)
  static unsigned int y = 0;

  y = y + dy;
  while (y >= HEIGHT) 
    y -= HEIGHT;

  wr_reg(0x6A, y);
  wr_reg(0x61, 3);
#endif
}
