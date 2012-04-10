# MPLAB IDE generated this makefile for use with GNU make.
# Project: Proj1.mcp
# Date: Wed Oct 22 19:08:06 2008

AS = mpasmwin.exe
CC = mcc18.exe
LD = mplink.exe
AR = mplib.exe
RM = rm

Proj1.cof : main.o messages.o interrupts.o user_interrupts.o my_uart.o uart_thread.o timer1_thread.o timer0_thread.o my_i2c.o
	$(LD) /l"C:\MCC18\lib" "..\..\..\..\MCC18\lkr\18f2680i.lkr" "main.o" "messages.o" "interrupts.o" "user_interrupts.o" "my_uart.o" "uart_thread.o" "timer1_thread.o" "timer0_thread.o" "my_i2c.o" /u_CRUNTIME /u_DEBUG /z__MPLAB_BUILD=1 /z__MPLAB_DEBUG=1 /m"Proj1.map" /w /o"Proj1.cof"

main.o : main.c ../../../../MCC18/h/stdio.h ../../../../MCC18/h/usart.h ../../../../MCC18/h/i2c.h ../../../../MCC18/h/timers.h messages.h my_uart.h my_i2c.h uart_thread.h timer1_thread.h timer0_thread.h main.c maindefs.h ../../../../MCC18/h/p18f2680.h ../../../../MCC18/h/stdarg.h ../../../../MCC18/h/stddef.h ../../../../MCC18/h/pconfig.h interrupts.h
	$(CC) -p=18F2680 "main.c" -fo="main.o" -D__DEBUG -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-

messages.o : messages.c messages.h messages.c maindefs.h ../../../../MCC18/h/p18f2680.h interrupts.h
	$(CC) -p=18F2680 "messages.c" -fo="messages.o" -D__DEBUG -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-

interrupts.o : interrupts.c messages.h my_uart.h interrupts.c maindefs.h ../../../../MCC18/h/p18f2680.h interrupts.h user_interrupts.h
	$(CC) -p=18F2680 "interrupts.c" -fo="interrupts.o" -D__DEBUG -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-

user_interrupts.o : user_interrupts.c ../../../../MCC18/h/timers.h messages.h my_uart.h user_interrupts.c maindefs.h ../../../../MCC18/h/p18f2680.h ../../../../MCC18/h/pconfig.h user_interrupts.h
	$(CC) -p=18F2680 "user_interrupts.c" -fo="user_interrupts.o" -D__DEBUG -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-

my_uart.o : my_uart.c ../../../../MCC18/h/usart.h messages.h my_uart.h my_uart.c maindefs.h ../../../../MCC18/h/p18f2680.h ../../../../MCC18/h/pconfig.h
	$(CC) -p=18F2680 "my_uart.c" -fo="my_uart.o" -D__DEBUG -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-

uart_thread.o : uart_thread.c ../../../../MCC18/h/stdio.h uart_thread.h uart_thread.c maindefs.h ../../../../MCC18/h/p18f2680.h ../../../../MCC18/h/stdarg.h ../../../../MCC18/h/stddef.h
	$(CC) -p=18F2680 "uart_thread.c" -fo="uart_thread.o" -D__DEBUG -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-

timer1_thread.o : timer1_thread.c ../../../../MCC18/h/stdio.h messages.h timer1_thread.h timer1_thread.c maindefs.h ../../../../MCC18/h/p18f2680.h ../../../../MCC18/h/stdarg.h ../../../../MCC18/h/stddef.h
	$(CC) -p=18F2680 "timer1_thread.c" -fo="timer1_thread.o" -D__DEBUG -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-

timer0_thread.o : timer0_thread.c ../../../../MCC18/h/stdio.h timer0_thread.h timer0_thread.c maindefs.h ../../../../MCC18/h/p18f2680.h ../../../../MCC18/h/stdarg.h ../../../../MCC18/h/stddef.h
	$(CC) -p=18F2680 "timer0_thread.c" -fo="timer0_thread.o" -D__DEBUG -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-

my_i2c.o : my_i2c.c ../../../../MCC18/h/i2c.h messages.h my_i2c.h my_i2c.c maindefs.h ../../../../MCC18/h/p18f2680.h ../../../../MCC18/h/pconfig.h
	$(CC) -p=18F2680 "my_i2c.c" -fo="my_i2c.o" -D__DEBUG -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-

clean : 
	$(RM) "main.o" "messages.o" "interrupts.o" "user_interrupts.o" "my_uart.o" "uart_thread.o" "timer1_thread.o" "timer0_thread.o" "my_i2c.o" "Proj1.cof" "Proj1.hex" "Proj1.map"

