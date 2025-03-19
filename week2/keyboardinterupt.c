#define RLEDs ((volatile long *) 0xFF200000)

volatile int * PS2_ptr = (int *) 0xFF200100;  // PS/2 port address
int makeNumbers[] = {0x45, 0x16, 0x1E, 0x26, 0x25, 0x2E, 0x36, 0x3D, 0x3E, 0x46};
int makeArrows[] = {0x75, 0x6B, 0x72, 0x74}; // up, left, down, right
int makeOther[] = {0x5A, 0x29, 0x66}; // enter, space, backspace 
int makeOtherE0[] = {0x05, 0x06, 0x04}; // F1, F2, F3
int PS2_data, RVALID;

unsigned char byte1 = 0;
unsigned char byte2 = 0;
unsigned char byte3 = 0;


void set_PS2();
void keyboard_handler();
void interrupt_handler();
void interrupt_setup();

void set_PS2()
{    
    // Keep reading while RVALID (bit 15) is set
    while (*PS2_ptr & 0x8000) {
        PS2_data = *PS2_ptr;  // Read and discard
    }

    *(PS2_ptr + 1) = 1; //enable interrupts RE bit
}


void keyboard_handler() {
    
		PS2_data = *(PS2_ptr);	// read the Data register in the PS/2 port
		// RVALID = (PS2_data & 0x8000);	// extract the RVALID field
		// if (RVALID != 0)
		// {
			/* always save the last three bytes received */
        byte1 = byte2;
        byte2 = byte3;
        byte3 = PS2_data & 0xFF;
		// }
		if ( (byte2 == 0xAA) && (byte3 == 0x00) )
		{
			// mouse inserted; initialize sending of data
			*(PS2_ptr) = 0xF4;
		}
		// Display last byte on Red LEDs
		

		if (byte2 == 0xE0) {
			for (int i = 0; i < 4; i++ ) {
				if (makeArrows[i] == byte3) {
					*RLEDs = i;
					break;
				}
			}
			// for (int i = 0; i < 3; i++ ) {
			// 	if (makeOtherE0[i] == byte3) {
			// 		*RLEDs = i;
			// 		break;
			// 	}
			// }
		}
		// else {
		// 	for (int i = 0; i < 10; i++ ) {
		// 		if (makeNumbers[i] == byte3) {
		// 			*RLEDs = i;
		// 			break;
		// 		}
		// 	}
		// 	for (int i = 0; i < 3; i++ ) {
		// 		if (makeOther[i] == byte3) {
		// 			*RLEDs = i;
		// 			break;
		// 		}
		// 	}
		// }

        // *(PS2_ptr + 1) = 0;

		return;
		// *RLEDs = byte3;


}

void interrupt_handler() {
    int mcause_value;
    __asm__ volatile ("csrr %0, mcause" : "=r"(mcause_value));

    *(PS2_ptr + 1) = 0; //clear interrupt

    if (mcause_value == 0x80000016) { //IRQ = 22
        keyboard_handler();
    }

    // *(PS2_ptr + 1) = 0;  // Clear the interrupt flag
    return;
}
void interrupt_setup()
{
    int mstatus_value, mtvec_value, mie_value;
    mstatus_value = 0b1000;
    __asm__ volatile ("csrc mstatus, %0" :: "r"(mstatus_value)); //clear mie enable
    mtvec_value = (int) &interrupt_handler;
    __asm__ volatile ("csrw mtvec, %0" :: "r"(mtvec_value)); //set handler address
    __asm__ volatile ("csrr %0, mie" : "=r"(mie_value));
    __asm__ volatile ("csrc mie, %0" :: "r"(mie_value)); //clear all current IRQ
    mie_value = 0x400000;
    __asm__ volatile ("csrs mie, %0" :: "r"(mie_value)); //set IRQ
    __asm__ volatile ("csrs mstatus, %0" :: "r"(mstatus_value)); //enable mie
}
int main() {
    interrupt_setup();
    set_PS2();

	while (1) {
        // __asm__ volatile ("wfi"); // Low-power wait for interrupt
	}
    return 0;
}