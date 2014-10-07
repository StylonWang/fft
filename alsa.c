#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

// Currently, we are a singleton.. really we should be returning this from
// capture_setup()
snd_pcm_t *capture_handle;

int capture_setup (char *device, unsigned int desired_rate)
{
    int err;
    snd_pcm_hw_params_t *hw_params;

    if ((err = snd_pcm_open (&capture_handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf (stderr, "cannot open audio device %s (%s)\n", 
             device,
             snd_strerror (err));
        return (-1);
    }
    // was _malloc
    snd_pcm_hw_params_alloca (&hw_params);
             
    if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
        fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
             snd_strerror (err));
        return (-1);
    }

    if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        fprintf (stderr, "cannot set access type (%s)\n",
             snd_strerror (err));
        return (-1);
    }

    if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
        fprintf (stderr, "cannot set sample format (%s)\n",
             snd_strerror (err));
        return (-1);
    }

    if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &desired_rate, 0)) < 0) {
        fprintf (stderr, "cannot set sample rate (%s)\n",
             snd_strerror (err));
        return (-1);
    }

    if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 1)) < 0) {
        fprintf (stderr, "cannot set channel count (%s)\n",
             snd_strerror (err));
        return (-1);
    }

    // Set number of periods. Periods used to be called fragments.
    int periods = 2; // 2 seems to be minimum?
    if ((err = snd_pcm_hw_params_set_periods(capture_handle, hw_params, periods, 0)) < 0) {
        fprintf (stderr, "cannot set period count (%s)\n",
             snd_strerror (err));
        return(-1);
    }

    // Set buffer size (in frames). The resulting latency is given by
    // latency = periodsize * periods / (rate * bytes_per_frame)
    snd_pcm_uframes_t periodsize = 8192; // bytes
    if (snd_pcm_hw_params_set_buffer_size(capture_handle, hw_params, (periodsize * periods)>>2) < 0) {
        fprintf(stderr, "Error setting buffersize.\n");
        return(-1);
    }
  

    if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
        fprintf (stderr, "cannot set parameters (%s)\n",
             snd_strerror (err));
        return (-1);
    }

    // snd_pcm_hw_params_free (hw_params);

    if ((err = snd_pcm_prepare (capture_handle)) < 0) {
        fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
             snd_strerror (err));
        return (-1);
    }

    return(0);
}

void capture_finish ()
{
    snd_pcm_close (capture_handle);
}


int capture_audio (short *buf, int data_size)
{

    int err = snd_pcm_readi (capture_handle, buf, data_size);

    if (err != data_size) {
        fprintf (stderr, "read from audio interface failed (%s)\n",
             snd_strerror (err));
        return (-1);
        // If a buffer overflow occurred, we need to call snd_pcm_prepare()
        // again.. I think..
    }

    return (0);
}
