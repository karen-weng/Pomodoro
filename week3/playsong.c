int samples[] = {0x00a1daf8,
    0xfdc16468,
    0xf5c8dc90,
    0xef98c320,
    0xefce6de0,
    0xf8a9df18,
    0xf8581ce0,
    0xf7732470};

int samples_n = 77920;

struct audio_t {
	volatile unsigned int control;
	volatile unsigned char rarc;
	volatile unsigned char ralc;
	volatile unsigned char warc;
	volatile unsigned char walc;
    volatile unsigned int ldata;
	volatile unsigned int rdata;
};

struct audio_t *const audiop = ((struct audio_t *)0xff203040);


void 
audio_playback_mono(int *samples, int n, int step, int replicate) {
            int i;

            audiop->control = 0x8; // clear the output FIFOs
            audiop->control = 0x0; // resume input conversion
            for (i = 0; i < n; i+=step) {
              // output data if there is space in the output FIFOs
              for (int r=0; r < replicate; r++) {
				while(audiop->warc == 0);
                audiop->ldata = samples[i];
                audiop->rdata = samples[i];
			  }
			}
	}	


int 
main(void) {
   audio_playback_mono(samples, samples_n, 1, 1);
   while (1);
}

