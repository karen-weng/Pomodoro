#define RLEDs ((volatile long *)0xFF200000)

volatile int *PS2_ptr = (int *)0xFF200100; // PS/2 port address
int makeNumbers[] = {0x45, 0x16, 0x1E, 0x26, 0x25, 0x2E, 0x36, 0x3D, 0x3E, 0x46};
int makeArrows[] = {0x75, 0x6B, 0x72, 0x74}; // up, left, down, right
int makeOther[] = {0x5A, 0x29, 0x66};        // enter, space, backspace
int makeOtherE0[] = {0x05, 0x06, 0x04};      // F1, F2, F3
int PS2_data, RVALID;

unsigned char byte1 = 0;
unsigned char byte2 = 0;
unsigned char byte3 = 0;

void set_PS2();
void interrupt_handler();
void interrupt_setup();

void set_PS2()
{
    // Keep reading while RVALID (bit 15) is set
    while (*PS2_ptr & 0x8000)
    {
        PS2_data = *PS2_ptr; // Read and discard
    }

    *(PS2_ptr + 1) = 1; // enable interrupts RE bit
}

void interrupt_handler()
{
    int mcause_value;
    __asm__ volatile("csrr %0, mcause" : "=r"(mcause_value));

    if (mcause_value == 0x80000016) { // IRQ = 22
        // Empty the entire FIFO to clear the interrupt
        while ((*PS2_ptr & 0x8000)) { // While RVALID is set
            PS2_data = *PS2_ptr;    // Read data (decrements RAVAIL)
            
            // Update byte history
            byte1 = byte2;
            byte2 = byte3;
            byte3 = PS2_data & 0xFF;
            
            // Process keyboard input - detect key releases
            // For standard keys, the release sequence is F0 followed by the key's make code
            if (byte2 == 0xF0) {
                // This is a key release event
                
                // Check if it's a number key
                for (int i = 0; i < 10; i++) {
                    if (byte3 == makeNumbers[i]) {
                        *RLEDs = i;
                        break;
                    }
                }
                
                // Check if it's another key
                for (int i = 0; i < 3; i++) {
                    if (byte3 == makeOther[i]) {
                        *RLEDs = 10 + i;  // Different LED patterns for these keys
                        break;
                    }
                }
            }
            
            // For E0-prefixed keys (like arrows), the release sequence is E0 F0 followed by the key's make code
            if (byte1 == 0xE0 && byte2 == 0xF0) {
                switch (byte3) {
                case 0x75: *RLEDs = 0x100; break; // Up released
                case 0x6B: *RLEDs = 0x200; break; // Left released
                case 0x72: *RLEDs = 0x400; break; // Down released
                case 0x74: *RLEDs = 0x800; break; // Right released
                }
                
                // Check for E0-prefixed function keys
                for (int i = 0; i < 3; i++) {
                    if (byte3 == makeOtherE0[i]) {
                        *RLEDs = 0x1000 + i;  // Different LED patterns for these keys
                        break;
                    }
                }
            }
        }
    }
    return;
}

void interrupt_setup()
{
    int mstatus_value, mtvec_value, mie_value;
    mstatus_value = 0b1000;
    __asm__ volatile("csrc mstatus, %0" ::"r"(mstatus_value)); // clear mie enable
    mtvec_value = (int)&interrupt_handler;
    __asm__ volatile("csrw mtvec, %0" ::"r"(mtvec_value)); // set handler address
    __asm__ volatile("csrr %0, mie" : "=r"(mie_value));
    __asm__ volatile("csrc mie, %0" ::"r"(mie_value)); // clear all current IRQ
    mie_value = 0x400000;
    __asm__ volatile("csrs mie, %0" ::"r"(mie_value));         // set IRQ
    __asm__ volatile("csrs mstatus, %0" ::"r"(mstatus_value)); // enable mie
}

int main()
{
    interrupt_setup();
    set_PS2();

    while (1)
    {
        // Main program loop - can be used for other tasks
        // The keyboard handling is done in the interrupt handler
    }
    return 0;
}