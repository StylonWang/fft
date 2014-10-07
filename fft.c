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

int main(int argc, char **argv)
{
    double *in;
    fftw_complex *out;
    fftw_plan p;
    const int N=128;
    int i;

    int fd;
    short buf[N*2];
    long processed_size=0;

    if(argc<2) {
        fprintf(stderr, "usage: %s raw-audio-file\n\n", argv[0]);
        fprintf(stderr, "audio format in file must be 16-bit signed little endian, 2 channel PCM audio\n");
        exit(1);
    }

    fd = open(argv[1], O_RDONLY);
    if(fd<0) {
        fprintf(stderr, "open failed: %s\n", strerror(errno));
        exit(1);
    }  

    in = (double*) fftw_malloc(sizeof(double) * N);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * ((N/2)+1));
    p = fftw_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);

    while(1) {
        ssize_t sz;
        
        sz = read(fd, buf, sizeof(buf));
        if(0==sz) break;
        else if(sz<0) {
            fprintf(stderr, "read failed: %s\n", strerror(errno));
            exit(1);
        }

        // add to processed sample count
        processed_size += sz/sizeof(buf[0])/2;

        // copy only 1 channel of audio into FFT in buffer
        for(i=0 ;i<(sz/sizeof(buf[0])/2); i++) {
            in[i] = buf[i*2];
        }
        // fill the remaing with zero, which happens just before end of file
        for(;i<N; ++i) {
            in[i] = 0;
        }

        fftw_execute(p);
    }

    fprintf(stderr, "result of %ld samples: ", processed_size);
#if 0
    for(i=0; i<(N/2+1); ++i) {
        if(i%8 == 0) { // pretty print
            fprintf(stdout, "\n%4d: ", i/8);
        }
        fprintf(stdout, "%6.3f ", cabs(out[i]));
    }
#else
    // CSV
    fprintf(stdout, "\n");
    for(i=0; i<(N/2+1); ++i) {
        fprintf(stdout, "%6.3f,", cabs(out[i]));
    }
#endif
    fprintf(stderr, "\n");

    fftw_free(in); 
    fftw_free(out);
    fftw_destroy_plan(p);
    close(fd);

    return 0;
}

