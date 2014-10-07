#ifndef __FFT_ALSA_H__
#define __FFT_ALSA_H__

int capture_setup (char *device, unsigned int desired_rate);
void capture_finish ();
int capture_audio (short *buf, int data_size);

#endif //__FFT_ALSA_H__
