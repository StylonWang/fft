
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <time.h>
#include <wiringPi.h>

#include "lcdband.h"

lcdband_t *lcdband_init(const char *spi_dev)
{
    int ret;
    int fd;
    lcdband_t *t=0;

    uint8_t mode = SPI_CPHA|SPI_CPOL; // clock high active, latch on rising edge
    uint8_t bits = 8;
    uint32_t speed = 500000;

    char clr[] = {0x80};
    char backlight[] = {0x8A, 30};

    // init wiringPi
    ret = wiringPiSetup();
    if(ret) {
        fprintf(stderr, "wiringPi set up failed: %d\n", ret);
        return NULL;
    }

    // physical pin #24 on Bananpi
    // man gpio for pin mapping.
    pinMode(6, 1);  // set to output mode

    // reset LCD
    digitalWrite(6, 1);
    usleep(1*1000);
    digitalWrite(6, 0);
    usleep(4*1000); // >2ms
    digitalWrite(6, 1);
    usleep(20*1000); // >10ms

    //fd = open("/dev/spidev0.0", O_RDWR);
    fd = open(spi_dev, O_RDWR);
    if(fd<0) {
        fprintf(stderr, "open device failed: %s\n", strerror(errno));
        return NULL;
    }

    // set SPI mode
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) {
        fprintf(stderr, "cannot set SPI mode: %s\n", strerror(errno));
        return NULL;
    } 

    // set bits per word
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1) {
        fprintf(stderr, "cannot set bits per word: %s\n", strerror(errno));
        return NULL;
    }

    // set speed
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
        fprintf(stderr, "cannot set speed: %s\n", strerror(errno));
        return NULL;
    }

    // clear screen
    write(fd, clr, sizeof(clr));

    // set backlight
    write(fd, backlight, sizeof(backlight));

    // allocate context
    t = malloc(sizeof(*t));
    if(!t) {
        fprintf(stderr, "malloc failed: %s\n", strerror(errno));
        return NULL;
    }

    memset(t, 0, sizeof(*t));
    t->fd = fd;

    return t;
}

int lcdband_set(lcdband_t *t, int band, int value)
{
    if(band>sizeof(t->band)/sizeof(t->band[0]) || band<0) {
        fprintf(stderr, "%s band %d exceeds limits\n", __func__, band);
        return -1;
    }

    t->band[band] = value;
    return 0;
}

// MzLH04 has an internal buffer of 400 bytes but no busy flag.
// Break up the writes and put some delay to avoid overflowing the buffer.
static ssize_t lcd_buffer_write(int fd, const void *buf, size_t count)
{
    ssize_t total = 0;
    int limit = 40;

    while(total<count) {

        if(count-total>limit) {
            ssize_t sz = write(fd, buf+total, limit);
            //printf("bwrite %ld\n", (long)sz);
            if(sz<0) return sz;
            else total += sz;
        }
        else {
            ssize_t sz = write(fd, buf+total, count-total);
            //printf("bwrite %ld\n", (long)sz);
            if(sz<0) return sz;
            else total += sz;
        }

        usleep(3*1000);
    }

    return total;
}

int lcdband_display(lcdband_t *t)
{
    int x, y, k, b;
    ssize_t sz;

    memset(t->display_buf, 0, sizeof(t->display_buf));
    // transform band info to display_buf
    for(b=0; b<sizeof(t->band)/sizeof(t->band[0]); ++b) {
        int v = t->band[b]/2;
        if(v>32) v=32;
        if(v<0) v=0;

        for(y=32-v; y<32; y++) {
            t->display_buf[y][b*2] = 1;
            t->display_buf[y][b*2+1] = 1;
        }
    }

    t->output_buf[0] = 0x0E; // show bit map
    t->output_buf[1] = 0; // X-coordinate: 0
    t->output_buf[2] = 0; // Y-coordinate: 0
    t->output_buf[3] = 128; // 128 pixels in width
    t->output_buf[4] = 32; // 32 pixels in height

    memset(t->output_buf+5, 0 ,sizeof(t->output_buf)-5);
    // transform display_buf to output_buf in bitmap
    for(y=0; y<32; ++y) {
        for(x=0; x<(128/8); ++x) {
            unsigned char *p = &t->display_buf[y][x*8];

            for(k=0; k<8; ++k) {
                t->output_buf[5+y*(128/8)+x] += (p[k] << (7-k));
            }
        }
    }

    sz = lcd_buffer_write(t->fd, t->output_buf, sizeof(t->output_buf));
    if(sz!=sizeof(t->output_buf)) {
        fprintf(stderr, "graphic write %ld instead of %d bytes\n", (long)sz, sizeof(t->output_buf));
        return -1;
    }

    return 0;
}

void lcdband_finish(lcdband_t *t)
{
    close(t->fd);
    free(t);
}

