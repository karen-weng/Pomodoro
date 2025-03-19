#define RLEDs ((volatile long *) 0xFF200000)

int main() {
	unsigned char byte1 = 0;
	unsigned char byte2 = 0;
	unsigned char byte3 = 0;
	
  	volatile int * PS2_ptr = (int *) 0xFF200100;  // PS/2 port address

	int makeNumbers[] = {0x45, 0x16, 0x1E, 0x26, 0x25, 0x2E, 0x36, 0x3D, 0x3E, 0x46};
	int makeArrows[] = {0x75, 0x6B, 0x72, 0x74}; // up, left, down, right
	int makeOther[] = {0x5A, 0x29, 0x66}; // enter, space, backspace 
	int makeOtherE0[] = {0x05, 0x06, 0x04}; // F1, F2, F3
	int PS2_data, RVALID;

	while (1) {
		PS2_data = *(PS2_ptr);	// read the Data register in the PS/2 port
		RVALID = (PS2_data & 0x8000);	// extract the RVALID field
		if (RVALID != 0)
		{
			/* always save the last three bytes received */
			byte1 = byte2;
			byte2 = byte3;
			byte3 = PS2_data & 0xFF;
		}
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
			for (int i = 0; i < 3; i++ ) {
				if (makeOtherE0[i] == byte3) {
					*RLEDs = i;
					break;
				}
			}
			
		}
		else {
			for (int i = 0; i < 10; i++ ) {
				if (makeNumbers[i] == byte3) {
					*RLEDs = i;
					break;
				}
			}
			for (int i = 0; i < 3; i++ ) {
				if (makeOther[i] == byte3) {
					*RLEDs = i;
					break;
				}
			}
		}
		
		// *RLEDs = byte3;
	}
}