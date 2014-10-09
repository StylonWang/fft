/*
 * Example: arecord -D plughw:1,0 --buffer-size=4410 --start-delay=2205 -r 48000 -c 2 -f S16_LE | ./fft-alsa | aplay
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <complex.h>
#include <fftw3.h>
#include <sys/select.h>
#include <time.h>
#include <pthread.h>
#include <math.h>

#include "lcdband.h"

#define N (128)
#define CH (2)

struct channel_data
{
    double *in;
    fftw_complex *out;
    fftw_plan p;

    double mag[N/2-1];
    int db[N/2-1];
} channels[CH];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 
long processed_size=0;

// determine how to display FFT results
#define SHOW_MAG
//#define SHOW_DB
#define SHOW_LCD

void *pthread_routine(void *data)
{
    lcdband_t *t;
    int ch;
    
    fprintf(stderr, "thread started\n");

    t = lcdband_init("/dev/spidev0.0");
    if(!t) return 0;

    while(1) {
        int i;
        struct timeval timeout;

        //timeout.tv_sec = 0;
        //timeout.tv_usec = 50*1000; // 50 mili seconds 
        timeout.tv_sec = 0;
        timeout.tv_usec = 100*1000; // 1 second
        select(0, NULL, NULL, NULL, &timeout);

        pthread_mutex_lock(&mutex);

#if defined(SHOW_MAG)
        for(ch=0; ch<CH; ++ch) {
            fprintf(stderr, "Magnitude[%d] %ld:\n", ch, processed_size);
            for(i=1; i<N/2+1; ++i) {
                if(i%8==0) fprintf(stderr, "\n");
                fprintf(stderr, "%6.3f, ", channels[ch].mag[i]);
            }
            fprintf(stderr, "\n");
        }
#endif

#if defined(SHOW_DB)
        for(ch=0; ch<CH; ++ch) {
            fprintf(stderr, "dB[%d] %ld:\n", chprocessed_size);
            for(i=1; i<N/2+1; ++i) {
                if(i%8==0) fprintf(stderr, "\n");
                fprintf(stderr, "%3d, ", channels[ch].db[i]);
            }
            fprintf(stderr, "\n");
        }
#endif

#if defined(SHOW_LCD)
        for(ch=0; ch<CH; ++ch) {
            for(i=1; i<(N/2+1); i+=2) { // show 64 bands in 32 bands
                int v;
                v = (channels[ch].mag[i]+channels[ch].mag[i+1])/2/1000;
                lcdband_set(t, ch, i/2, v);
            }
        }

        lcdband_display(t);
#endif

        pthread_mutex_unlock(&mutex);

    }

    fprintf(stderr, "thread ended\n");

    lcdband_finish(t);

    return 0;
}

int main(int argc, char **argv)
{
    int i;
    int ch;
    int fd;

    short buf[N*2];
    pthread_t th;

    if(argc>1 && strncmp(argv[1], "-h", 2)==0) {
        fprintf(stderr, "usage: arecord | %s | aplay \n\n", argv[0]);
        fprintf(stderr, "audio format must be 16-bit signed little endian, 2 channel PCM audio\n");
        exit(1);
    }

    fd = STDIN_FILENO;

    for(ch=0; ch<CH; ++ch) {

        channels[ch].in = (double*) fftw_malloc(sizeof(double) * N);
        if(!channels[ch].in) {
            fprintf(stderr, "ch %d unable to alloc in\n", ch);
            exit(1);
        }

        channels[ch].out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * ((N/2)+1));
        if(!channels[ch].out) {
            fprintf(stderr, "ch %d unable to alloc out\n", ch);
            exit(1);
        }

        channels[ch].p = fftw_plan_dft_r2c_1d(N, channels[ch].in, channels[ch].out, FFTW_ESTIMATE);
        if(!channels[ch].p) {
            fprintf(stderr, "ch %d unable to alloc p\n", ch);
            exit(1);
        }

    }

    pthread_create(&th, NULL, pthread_routine, NULL);

    while(1) {
        ssize_t sz;
        
        sz = read(fd, buf, sizeof(buf));
        if(0==sz) {
            fprintf(stderr, "EOF\n");
            break;
        }
        else if(sz<0) {
            fprintf(stderr, "read failed: %s\n", strerror(errno));
            break;
        }

        // add to processed sample count
        processed_size += sz/sizeof(buf[0])/CH;

        // copy channels of audio into FFT in buffer
        for(ch=0; ch<CH; ++ch) {
            for(i=0 ;i<(sz/sizeof(buf[0])/CH); i++) {
                //in[i] = buf[i*2];
                channels[ch].in[i] = buf[i*CH+ch];
            }
            // fill the remaining with zero, which happens just before end of file
            for(;i<sizeof(buf)/sizeof(buf[0])/CH; ++i) {
                //in[i] = 0;
                channels[ch].in[i] = 0;
            }

            fftw_execute(channels[ch].p);
        }


#if 1
        // calculate result for thread to display
        pthread_mutex_lock(&mutex);
        for(ch=0; ch<CH; ++ch) {
            for(i=0; i<(N/2+1); ++i) {
                channels[ch].mag[i] = sqrtf((creal(channels[ch].out[i]) * creal(channels[ch].out[i])) + (cimag(channels[ch].out[i]) * cimag(channels[ch].out[i])));
        //        channels[ch].db[i] = (10.0f * log10f(channels[ch].mag[i] + 1.0));
            }
        }
        pthread_mutex_unlock(&mutex);
#endif
        // write to stdout
        write(STDOUT_FILENO, buf, sz);
    }

    for(ch=0; ch<CH; ++ch) {
        fftw_free(channels[ch].in); 
        fftw_free(channels[ch].out);
        fftw_destroy_plan(channels[ch].p);
    }
    close(fd);

    fprintf(stderr, "ended\n");
    return 0;
}

