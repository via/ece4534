#include "vtUtilities.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lpc17xx_gpio.h"

// This tells us which pins on the board are the GPIO ports for the LEDs (printed on the board)
// P2.2 thru P2.6
#define partstFIO2_BITS			( ( unsigned long ) 0x0000007C )
#define P2Mask1 (partstFIO2_BITS & 0x54)
#define P2Mask2 (partstFIO2_BITS & 0x28)
// P1.28 thru P1.31
#define partstFIO1_BITS			( ( unsigned long ) 0xB0000000 )
#define P1Mask1 (partstFIO1_BITS & 0x20000000)
#define P1Mask2 (partstFIO1_BITS & 0x90000000)
// By using a macro (which uses some C pre-processor magic) I pass three things to this fatal error handler
// 1. A "code", which can be whatever the caller wants
// 2. The line number of the code that called the macro
// 3. The file in which the macro was called
// You can view all three of those values if you use the debugger (after this routine is called)

void vtInitLED()
{
	/* LEDs on ports 1 and 2 to output (1). */
	// Note that all LED access is through the proper LPC library calls
	GPIO_SetDir(1,partstFIO1_BITS,1);
	GPIO_SetDir(2,partstFIO2_BITS,1);

	/* Start will all LEDs off. */
	GPIO_ClearValue(1,partstFIO1_BITS);
	GPIO_ClearValue(2,partstFIO2_BITS);

}

// The LED routines use an unsigned byte bitmask for the 8 LEDs on the board
//   The LSB is LED P2.6
//   The MSB is LED P1.28 (see the board for what this means)
void vtLEDOn(uint8_t mask)
{
	if (mask & 0x80) {
		// LED P1.28
		GPIO_SetValue(1,0x10000000);
	}
	if (mask & 0x40) {
		// LED P1.29
		GPIO_SetValue(1,0x20000000);
	}
	if (mask & 0x20) {
		// LED P1.31
		GPIO_SetValue(1,0x80000000);
	}
	if (mask & 0x10) {
		// LED P2.2
		GPIO_SetValue(2,0x00000004);
	}
	if (mask & 0x08) {
		// LED P2.3
		GPIO_SetValue(2,0x00000008);
	}
	if (mask & 0x04) {
		// LED P2.4
		GPIO_SetValue(2,0x00000010);
	}
	if (mask & 0x02) {
		// LED P2.5
		GPIO_SetValue(2,0x00000020);
	}
	if (mask & 0x01) {
		// LED P2.6
		GPIO_SetValue(2,0x00000040);
	}
}

void vtLEDOff(uint8_t mask)
{
	if (mask & 0x80) {
		// LED P1.28
		GPIO_ClearValue(1,0x10000000);
	}
	if (mask & 0x40) {
		// LED P1.29
		GPIO_ClearValue(1,0x20000000);
	}
	if (mask & 0x20) {
		// LED P1.31
		GPIO_ClearValue(1,0x80000000);
	}
	if (mask & 0x10) {
		// LED P2.2
		GPIO_ClearValue(2,0x00000004);
	}
	if (mask & 0x08) {
		// LED P2.3
		GPIO_ClearValue(2,0x00000008);
	}
	if (mask & 0x04) {
		// LED P2.4
		GPIO_ClearValue(2,0x00000010);
	}
	if (mask & 0x02) {
		// LED P2.5
		GPIO_ClearValue(2,0x00000020);
	}
	if (mask & 0x01) {
		// LED P2.6
		GPIO_ClearValue(2,0x00000040);
	}
}

void vtHandleFatalError(int code,int line,char file[]) {
	static unsigned int delayCounter = 0;
	// There are lots of ways you can (and may want to) handle a fatal error
	// In this implementation, I suspend tasks and then flash the LEDs.  Note that the stop may not be
	//   immediate because this task has to be scheduled first for that to happen.  In fact, one task might
	//   call this while another tries to get into 
	taskENTER_CRITICAL();
	taskDISABLE_INTERRUPTS();
	/* LEDs on ports 1 and 2 to output (1). */
	// Note that all LED access is through the proper LPC library calls (or my own routines that call them)
	vtInitLED();
	for (;;) {
		// Silly delay loop, but we really don't have much choice here w/o interrupts and FreeRTOS...
		// This won't be okay to do *anywhere* else in your code
		for (delayCounter=0;delayCounter<10000000;delayCounter++) {
		}
		// Turn off half and on half
		vtLEDOn(0xAA);
		vtLEDOff(0x55);
		// Delay again
		for (delayCounter=0;delayCounter<10000000;delayCounter++) {
		}
		// Toggle
		vtLEDOff(0xAA);
		vtLEDOn(0x55);
		// Here is some dumb code to make sure that the three input parameters are not optimized away by the compiler
		if ((code < -55) && (line < -55) && (file == NULL)) {
			vtLEDOff(0x0); // We won't get here
		}
		// End of dumb code
	}
	// We will never get here
	taskEXIT_CRITICAL();
}

// Only need to define this for the USB destination -- otherwise it is just a macro in VTutilities.h
#if PRINTF_DESTINATION==1
#include <stdio.h>
#include "extUSB.h"
int VTputchar(int c)
{
	if (c == '\n') {
		if (writeUSBChar('\r') == EOF) return(EOF);
	} 
	return(writeUSBChar(c));
}
#endif

// define a version of printf() that goes to the debugging port (ITM 0)
#if ((PRINTF_VERSION == 2) || (PRINTF_VERSION == 0))
/*
	Copyright 2001, 2002 Georges Menie (www.menie.org)
	stdarg version contributed by Christian Ettinger

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
	VTputchar is the only external dependency for this file,
	if you have a working putchar, leave it commented out.
	If not, uncomment the define below and
	replace outbyte(c) by your own function call.

*/

#include <stdarg.h>
int putchar(int c)
{
	return VTputchar(c);
}

static void printchar(char **str, int c)
{
	//extern int putchar(int c);
	
	if (str) {
		**str = (char)c;
		++(*str);
	}
	else
	{ 
		(void)VTputchar(c);
	}
}

#define PAD_RIGHT 1
#define PAD_ZERO 2

static int prints(char **out, const char *string, int width, int pad)
{
	register int pc = 0, padchar = ' ';

	if (width > 0) {
		register int len = 0;
		register const char *ptr;
		for (ptr = string; *ptr; ++ptr) ++len;
		if (len >= width) width = 0;
		else width -= len;
		if (pad & PAD_ZERO) padchar = '0';
	}
	if (!(pad & PAD_RIGHT)) {
		for ( ; width > 0; --width) {
			printchar (out, padchar);
			++pc;
		}
	}
	for ( ; *string ; ++string) {
		printchar (out, *string);
		++pc;
	}
	for ( ; width > 0; --width) {
		printchar (out, padchar);
		++pc;
	}

	return pc;
}

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12

static int printi(char **out, int i, int b, int sg, int width, int pad, int letbase)
{
	char print_buf[PRINT_BUF_LEN];
	register char *s;
	register int t, neg = 0, pc = 0;
	register unsigned int u = (unsigned int)i;

	if (i == 0) {
		print_buf[0] = '0';
		print_buf[1] = '\0';
		return prints (out, print_buf, width, pad);
	}

	if (sg && b == 10 && i < 0) {
		neg = 1;
		u = (unsigned int)-i;
	}

	s = print_buf + PRINT_BUF_LEN-1;
	*s = '\0';

	while (u) {
		t = (unsigned int)u % b;
		if( t >= 10 )
			t += letbase - '0' - 10;
		*--s = (char)(t + '0');
		u /= b;
	}

	if (neg) {
		if( width && (pad & PAD_ZERO) ) {
			printchar (out, '-');
			++pc;
			--width;
		}
		else {
			*--s = '-';
		}
	}

	return pc + prints (out, s, width, pad);
}

static int print( char **out, const char *format, va_list args )
{
	register int width, pad;
	register int pc = 0;
	char scr[2];

	for (; *format != 0; ++format) {
		if (*format == '%') {
			++format;
			width = pad = 0;
			if (*format == '\0') break;
			if (*format == '%') goto out;
			if (*format == '-') {
				++format;
				pad = PAD_RIGHT;
			}
			while (*format == '0') {
				++format;
				pad |= PAD_ZERO;
			}
			for ( ; *format >= '0' && *format <= '9'; ++format) {
				width *= 10;
				width += *format - '0';
			}
			if( *format == 's' ) {
				register char *s = (char *)va_arg( args, int );
				pc += prints (out, s?s:"(null)", width, pad);
				continue;
			}
			if( *format == 'd' ) {
				pc += printi (out, va_arg( args, int ), 10, 1, width, pad, 'a');
				continue;
			}
			if( *format == 'x' ) {
				pc += printi (out, va_arg( args, int ), 16, 0, width, pad, 'a');
				continue;
			}
			if( *format == 'X' ) {
				pc += printi (out, va_arg( args, int ), 16, 0, width, pad, 'A');
				continue;
			}
			if( *format == 'u' ) {
				pc += printi (out, va_arg( args, int ), 10, 0, width, pad, 'a');
				continue;
			}
			if( *format == 'c' ) {
				/* char are converted to int then pushed on the stack */
				scr[0] = (char)va_arg( args, int );
				scr[1] = '\0';
				pc += prints (out, scr, width, pad);
				continue;
			}
		}
		else {
		out:
			printchar (out, *format);
			++pc;
		}
	}
	if (out) **out = '\0';
	va_end( args );
	return pc;
}
#if PRINTF_VERSION == 2
int printf(const char *format, ...)
{
        va_list args;
        
        va_start( args, format );
        return print( 0, format, args );
}
int puts(const char *s)
{
	int pc = 0;
	pc += prints(0,s,0,0);
	pc += prints(0,"\n",0,0);
	return pc;
}	
#else
int printf(const char *format, ...)
{
        return(0);
}
int (puts(const char *s)
{
	return(0);
}
#endif
int sprintf(char *out, const char *format, ...)
{
        va_list args;
        
        va_start( args, format );
        return print( &out, format, args );
}


int snprintf( char *buf, unsigned int count, const char *format, ... )
{
        va_list args;
        
        ( void ) count;
        
        va_start( args, format );
        return print( &buf, format, args );
}
#endif

#if MALLOC_VERSION==1
#include "heap_3.c"
#elif MALLOC_VERSION==2
#include "heap_2.c"
#endif