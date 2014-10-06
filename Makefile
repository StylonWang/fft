
CROSS_COMPILE=
CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall
LDFLAGS=-lfftw3 -lm

default:: fft

fft:: fft.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

