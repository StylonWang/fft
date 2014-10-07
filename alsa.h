
int capture_setup (char *device, unsigned int desired_rate);
void capture_finish ();
int capture_audio (short *buf, int data_size);
