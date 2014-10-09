#ifndef __LCD_BAND_H__
#define __LCD_BAND_H__

typedef struct lcdband_t 
{
    int fd; // fd to spi lcd device
    unsigned char band[32];

    unsigned char output_buf[5 + 128*(32/8)]; // cmd: 1byte, x-y-w-h: 4 bytes
    unsigned char display_buf[32][128];

} lcdband_t;

lcdband_t *lcdband_init(const char *spi_dev);
void lcdband_finish(lcdband_t *t);

int lcdband_set(lcdband_t *t, int band, int value);

int lcdband_display(lcdband_t *t);

#endif //__LCD_BAND_H__
