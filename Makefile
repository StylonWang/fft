
CROSS_COMPILE=
CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall -g
LDFLAGS=-lfftw3 -lm -lasound -lpthread

default:: fft fft-alsa

clean::
	rm -f fft fft-alsa

fft:: fft.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

fft-alsa:: fft-alsa.c 
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

