
CROSS_COMPILE=
CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall
LDFLAGS=-lfftw3 -lm

default:: fft fft-alsa

clean::
	rm -f fft fft-alsa

fft:: fft.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

fft-alsa:: fft-alsa.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

