#define AUDIO_BASE 0xFF203040

int main(void) {
    volatile int *audio_ptr = (int *) AUDIO_BASE;

    int fifospace;
    int signal_val = 0x7FFFFF;  // Max amplitude
    int counter = 0;
    int frequency = 440; // Set desired frequency (e.g., 440Hz for A4 note)

    // Calculate number of samples per half-period
    int numSamples = (int)(1.0 / (2 * frequency * 0.000125)); 

    while (1) {
        fifospace = *(audio_ptr + 1); // Read FIFO space register
        if ((fifospace & 0x000000FF) > 0) { // Check if data is available
            *(audio_ptr + 2) = signal_val; // Left channel
            *(audio_ptr + 3) = signal_val; // Right channel

            counter++;

            if (counter >= numSamples) { // Toggle signal every half-period
                signal_val = (signal_val == 0x7FFFFF) ? 0 : 0x7FFFFF;
                counter = 0;
            }
        }
    }
}

