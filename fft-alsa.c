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

#define N (128)

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 
double mag[N/2-1];
int db[N/2-1];
long processed_size=0;

void *pthread_routine(void *data)
{
    fprintf(stderr, "thread started\n");
    while(1) {
        int i;
        struct timeval timeout;

        //timeout.tv_sec = 0;
        //timeout.tv_usec = 50*1000; // 50 mili seconds 
        timeout.tv_sec = 1;
        timeout.tv_usec = 0; // 1 second
        select(0, NULL, NULL, NULL, &timeout);

        //TODO: draw spectrum based on out2
        pthread_mutex_lock(&mutex);

#if 1
        fprintf(stderr, "Magnitude %ld:\n", processed_size);
        for(i=0; i<N/2+1; ++i) {
            if(i%8==0) fprintf(stderr, "\n");
            fprintf(stderr, "%6.3f, ", mag[i]);
        }
        fprintf(stderr, "\n");
#else
        fprintf(stderr, "dB %ld:\n", processed_size);
        for(i=0; i<N/2+1; ++i) {
            if(i%8==0) fprintf(stderr, "\n");
            fprintf(stderr, "%3d, ", db[i]);
        }
        fprintf(stderr, "\n");
#endif
        pthread_mutex_unlock(&mutex);

    }

    fprintf(stderr, "thread ended\n");
    return 0;
}

int main(int argc, char **argv)
{
    double *in;
    fftw_complex *out, *out2;
    fftw_plan p;
    int i;

    int fd;
    short buf[N*2];

    pthread_t th;

    if(argc>1 && strncmp(argv[1], "-h", 2)==0) {
        fprintf(stderr, "usage: arecord | %s | aplay \n\n", argv[0]);
        fprintf(stderr, "audio format must be 16-bit signed little endian, 2 channel PCM audio\n");
        exit(1);
    }

    fd = STDIN_FILENO;

    in = (double*) fftw_malloc(sizeof(double) * N);
    if(!in) {
        fprintf(stderr, "unable to alloc in\n");
        exit(1);
    }

    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * ((N/2)+1));
    if(!out) {
        fprintf(stderr, "unable to alloc out\n");
        exit(1);
    }

    out2 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * ((N/2)+1));
    if(!out2) {
        fprintf(stderr, "unable to alloc out2\n");
        exit(1);
    }

    p = fftw_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);
    if(!p) {
        fprintf(stderr, "unable to alloc p\n");
        exit(1);
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
        processed_size += sz/sizeof(buf[0])/2;

        // copy only 1 channel of audio into FFT in buffer
        for(i=0 ;i<(sz/sizeof(buf[0])/2); i++) {
            in[i] = buf[i*2];
        }
        // fill the remaining with zero, which happens just before end of file
        for(;i<N/2-1; ++i) {
            in[i] = 0;
        }

        fftw_execute(p);

#if 1
        // calculate result for thread to display
        pthread_mutex_lock(&mutex);
        for(i=0; i<(N/2+1); ++i) {
            mag[i] = sqrtf((creal(out[i]) * creal(out[i])) + (cimag(out[i]) * cimag(out[i])));
            db[i] = (10.0f * log10f(mag[i] + 1.0));
        }
        pthread_mutex_unlock(&mutex);
#endif
        // write to stdout
        write(STDOUT_FILENO, buf, sz);
    }

    fftw_free(in); 
    fftw_free(out);
    fftw_free(out2);
    fftw_destroy_plan(p);
    close(fd);

    fprintf(stderr, "ended\n");
    return 0;
}

