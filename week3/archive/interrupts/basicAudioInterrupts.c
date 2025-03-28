#define AUDIO_BASE 0xFF203040

// Global variables
volatile int signal_val = 0x7FFFFF; // Max amplitude
volatile int counter = 0;           // Counter for toggling the wave
volatile int frequency = 440;       // Default frequency (e.g., 440Hz for A4 note)

// Function prototypes
void set_AUDIO(void);
void audio_ISR(void);
void handler(void);

int main(void) {
    // Configure the audio system
    set_AUDIO();

    // Enable interrupts
    int mstatus_value, mtvec_value, mie_value;
    mstatus_value = 0b1000; // Enable global interrupts
    __asm__ volatile ("csrc mstatus, %0" :: "r"(mstatus_value));
    mtvec_value = (int)&handler; // Set trap address
    __asm__ volatile ("csrw mtvec, %0" :: "r"(mtvec_value));
    mie_value = 0x00200000; // Enable audio interrupt (IRQ 21, one-hot encoding)
    __asm__ volatile ("csrs mie, %0" :: "r"(mie_value));
    __asm__ volatile ("csrs mstatus, %0" :: "r"(mstatus_value));

    // Main loop (idle)
    while (1) {
        // The audio playback is handled entirely in the interrupt
    }
}

// Configure the audio system
void set_AUDIO(void) {
    volatile int *audio_ptr = (int *)AUDIO_BASE;
    *(audio_ptr) = 0b1010; // Clear FIFOs and enable write interrupts (WE)
}

// Audio interrupt service routine
void audio_ISR(void) {
    volatile int *audio_ptr = (int *)AUDIO_BASE;

    // Check if the write FIFO has space
    int fifospace = *(audio_ptr + 1); // Read the audio FIFO space register

    // While the FIFO space is less than or equal to 32, fill the FIFO
    if ((fifospace & 0x00FF) <= 32) {
        // Calculate the number of samples per half-period dynamically
        int numSamples = (int)(1.0 / (2 * frequency * 0.000125)); // 0.000125 = 1/8kHz

        // Write the current signal value to the audio channels
        *(audio_ptr + 2) = signal_val; // Left channel
        *(audio_ptr + 3) = signal_val; // Right channel

        counter++;

        // Toggle the signal value every half-period
        if (counter >= numSamples) {
            signal_val *= -1; // Flip the signal value
            counter = 0;      // Reset the counter
        }

        // Update FIFO space
        fifospace = *(audio_ptr + 1); // Recheck available space after writing
    }

    // Clear the interrupt flag and re-enable write interrupts
    *(audio_ptr) = 0b1010; // Clear FIFOs and re-enable write interrupts
}

// Trap handler for interrupts
void handler(void) {
    int mcause_value;
    __asm__ volatile ("csrr %0, mcause" : "=r"(mcause_value));
    if (mcause_value == 0x80000015) { // Audio FIFO interrupt (IRQ 21, one-hot encoding)
        audio_ISR();
    } else {
        // Handle other interrupts or unexpected traps
    }
}