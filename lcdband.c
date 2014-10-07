
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

typedef struct lcdband_t 
{
    int fd; // fd to spi lcd device
    unsigned char band[64];

} lcdband_t;

lcdband_t *lcdband_init(const char *spi_dev)
{
    int ret;
    int fd;
    lcdband_t *t=0;

    uint8_t mode = SPI_CPHA|SPI_CPOL; // clock high active, latch on rising edge
    uint8_t bits = 8;
    uint32_t speed = 500000;

    char clr[] = {0x80};
    char backlight[] = {0x8A, 10};

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
    t->band[band] = value;
    return 0;
}

int lcdband_display(lcdband_t *t)
{

}

void lcdband_finish(lcdband_t *t)
{
    close(t->fd);
    free(t);
}

