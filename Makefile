
CROSS_COMPILE=
CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall
LDFLAGS=-lfftw3 -lm

default:: fft

clean::
	rm -f fft

fft:: fft.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

