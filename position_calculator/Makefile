all: locatelib.o test_locatelib

CFLAGS=-Wall  -O0 -ggdb -lm

locatelib.o: locatelib.c locatelib.h
	gcc -o locatelib.o -c locatelib.c ${CFLAGS}

test_locatelib: locatelib.o test_locatelib.c
	gcc -o test_locatelib test_locatelib.c locatelib.o ${CFLAGS}

clean:
	-rm *.o test_locatelib


run: test_locatelib input
	./test_locatelib <input >output
	echo set autoscale\; set terminal png\; set output \'out.png\'\; plot \"output\" using 1:2\; quit | gnuplot

				
