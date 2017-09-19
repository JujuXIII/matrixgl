INCLUDE = -I/usr/X11R6/include/ 
LIBDIR  = -L/usr/X11R6/lib 

CFLAGS = -g -Wall -pedantic -ansi -W -Wno-unused
CC = gcc
LIBRARIES = -lX11 -lEGL -lGL -lm

Matrix: matrix.o
	$(CC) matrix.o -o matrixgl $(CFLAGS) $(INCLUDE) $(LIBDIR) $(LIBRARIES)
	strip matrixgl
	@rm -f *.o

matrix: matrix.c
	$(CC) -c $(CFLAGS) matrix.c
