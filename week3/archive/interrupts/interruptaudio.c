#define BOARD				"DE1-SoC"
 
/* Memory */
#define DDR_BASE			0x40000000
#define DDR_END				0x7FFFFFFF
#define A9_ONCHIP_BASE			0xFFFF0000
#define A9_ONCHIP_END			0xFFFFFFFF
#define SDRAM_BASE			0x00000000
#define SDRAM_END			0x03FFFFFF
#define FPGA_PIXEL_BUF_BASE		0x08000000
#define FPGA_PIXEL_BUF_END		0x0803FFFF
#define FPGA_CHAR_BASE			0x099999999
#define FPGA_CHAR_END			0x09001FFF

/* Cyclone V FPGA devices */
#define LED_BASE			0xFF200000
#define LEDR_BASE			0xFF200000
#define HEX3_HEX0_BASE			0xFF200020
#define HEX5_HEX4_BASE			0xFF200030
#define SW_BASE				0xFF200040
#define KEY_BASE			0xFF200050
#define JP1_BASE			0xFF200060
#define JP2_BASE			0xFF200070
#define PS2_BASE			0xFF200100
#define PS2_DUAL_BASE			0xFF200108
#define JTAG_UART_BASE			0xFF201000
#define IrDA_BASE			0xFF201020
#define TIMER_BASE			0xFF202000
#define TIMER_2_BASE			0xFF202020
#define AV_CONFIG_BASE			0xFF203000
#define RGB_RESAMPLER_BASE    		0xFF203010
#define PIXEL_BUF_CTRL_BASE		0xFF203020
#define CHAR_BUF_CTRL_BASE		0xFF203030
#define AUDIO_BASE			0xFF203040
#define VIDEO_IN_BASE			0xFF203060
#define EDGE_DETECT_CTRL_BASE		0xFF203070
#define ADC_BASE			0xFF204000

/* Cyclone V HPS devices */
#define HPS_GPIO1_BASE			0xFF709000
#define I2C0_BASE			0xFFC04000
#define I2C1_BASE			0xFFC05000
#define I2C2_BASE			0xFFC06000
#define I2C3_BASE			0xFFC07000
#define HPS_TIMER0_BASE			0xFFC08000
#define HPS_TIMER1_BASE			0xFFC09000
#define HPS_TIMER2_BASE			0xFFD00000
#define HPS_TIMER3_BASE			0xFFD01000
#define FPGA_BRIDGE			0xFFD0501C

//#include "address_map_niosv.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define clock_rate 100000000
#define quarter_clock clock_rate / 4

static void handler(void) __attribute__ ((interrupt ("machine")));
void set_itimer(void);
void set_KEY(void);
void itimer_ISR(void);
void KEY_ISR(void);

void set_PS2();
void PS2_ISR(void); // IRQ = 22


void pressed_enter(void);
void play_alarm(void);

void set_AUDIO(void); // IRQ = 21
void audio_ISR(void);

// keyboard variables
volatile int *PS2_ptr = (int *)0xFF200100; // PS/2 port address
volatile unsigned char PS2_data;
volatile unsigned char RVALID;
volatile int led_display_val = 0;

volatile unsigned char byte1 = 0;
volatile unsigned char byte2 = 0;
volatile unsigned char byte3 = 0;

// Audio codec Register address
volatile int * AUDIO_ptr = (int *) AUDIO_BASE;
volatile int * SW_ptr = (int *) SW_BASE;

int left, right, fifospace;
// int delay_samples = 3200;
int alarm_audio_left[100000];
int alarm_audio_right[100000];
int audio_array_index = 0;
bool recording = false;
// int front = 0;  // Index of the first element
// int rear = 0;   // Index where the next element will be added
// int sample_counter = 0;  // Number of elements in the array

// numSamples = 0.5s / (125us * Hz)
// 40 samples half period is 100hz, 2 samples is 2khz
volatile int frequency = 440;
volatile int signal_val = 0x7FFFFFF;
volatile int counter = 0;
// int numSamples = 40; 



// timer/hannalee variables
volatile int pom_start_val = 25;
volatile int small_break_start_val = 5;
volatile int big_break_start_val = 15;
volatile int sec_time = 0;
volatile int min_time = 0;
volatile int key_mode = 1;  // 1 for start, 2 for pause, 3 for stop (ringing) --> multiply by 2 for break vals
volatile bool study_mode = true; // 0 for pomodoro, 1 for break, 2 for long break
volatile int study_session_count = 1;

volatile int *LEDR_ptr = (int *) LEDR_BASE;
volatile int *HEX3_HEX0_ptr = (int *) HEX3_HEX0_BASE;
volatile int *TIMER_ptr = (int *) TIMER_BASE;
volatile int *KEY_ptr = (int *) KEY_BASE;

void plot_pixel(int, int, short int); // plots one pixel
void clear_screen(); // clears whole screen
void display_num(int, int, short int, int);
void countdown_display(int, int*);
void wait_for_v_sync();
volatile int* PIXEL_BUF_CTRL_ptr = (int*) PIXEL_BUF_CTRL_BASE;
int pixel_buffer_start; // global variable
short int buffer1[240][512]; // 240 rows, 512 (320 + padding) columns
short int buffer2[240][512];

int main(void) {
    /* Declare volatile pointers to I/O registers (volatile means that the
     * accesses will always go to the memory (I/O) address */

    set_itimer();
    set_KEY();
    set_PS2();
    set_AUDIO();

    int mstatus_value, mtvec_value, mie_value;
    mstatus_value = 0b1000; // interrupt bit mask
    // disable interrupts
    __asm__ volatile ("csrc mstatus, %0" :: "r"(mstatus_value));
    mtvec_value = (int) &handler; // set trap address
    __asm__ volatile ("csrw mtvec, %0" :: "r"(mtvec_value));
    // disable all interrupts that are currently enabled
    __asm__ volatile ("csrr %0, mie" : "=r"(mie_value));
    __asm__ volatile ("csrc mie, %0" :: "r"(mie_value));
    mie_value = 0x450002; // KEY, itimer, ps/2 port (non-dual) -- irq 16, 18, 21, 22
    // set interrupt enables
    __asm__ volatile ("csrs mie, %0" :: "r"(mie_value));
    // enable Nios V interrupts
    __asm__ volatile ("csrs mstatus, %0" :: "r"(mstatus_value));

    sec_time = pom_start_val;

    /* set front pixel buffer to buffer 1 */
    *(PIXEL_BUF_CTRL_ptr + 1) = (int) &buffer1; // first store the address in the  back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_v_sync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *PIXEL_BUF_CTRL_ptr;
    // clear_screen(); // pixel_buffer_start points to the pixel buffer
    /* set back pixel buffer to buffer 2 */
    *(PIXEL_BUF_CTRL_ptr + 1) = (int) &buffer2;
    pixel_buffer_start = *(PIXEL_BUF_CTRL_ptr + 1); // we draw on the back buffer
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    
    int n = 0;
    int count = 0;
    int digits [2];
    while (1) {
        countdown_display(sec_time, digits);
        wait_for_v_sync();
        pixel_buffer_start = *(PIXEL_BUF_CTRL_ptr + 1); // new back buffer
        clear_screen();
        display_num(120, 100, 0xFFFF, digits[1]);
        display_num(150, 100, 0xFFFF, digits[0]);
        count++;

        *LEDR_ptr = led_display_val;


        // fifospace = *(AUDIO_ptr + 1);
        
// clear fifospace at some point
        // if (recording && audio_array_index < 100000) {
        //     *LEDR_ptr = 0xF;
        //     // fifospace = *(AUDIO_ptr + 1); // read the audio port fifospace register
        //     if ((fifospace & 0x000000FF) > 0) // check RARC to see if there is data to read
        //     {
        //         // load both input microphone channels - just get one sample from each
        //         int left = *(AUDIO_ptr + 2);
        //         int right = *(AUDIO_ptr + 3);
        //         alarm_audio_left[audio_array_index] = left;
        //         alarm_audio_right[audio_array_index] = right;

        //         *(AUDIO_ptr + 2) = alarm_audio_left[audio_array_index];
        //         *(AUDIO_ptr + 3) = alarm_audio_right[audio_array_index];

        //     }
        //     audio_array_index++;
        // }

        // if (key_mode == 3) {
        //     *(AUDIO_ptr + 2) = signal_val; // Left channel
        //     *(AUDIO_ptr + 3) = signal_val; // Right channel

        //     counter++;

        //     if (counter >= numSamples) { // toggle high low
        //         if (signal_val == 99999999) {
        //             signal_val = -99999999;
        //         }
        //         else {
        //             signal_val = 99999999;
        //         }
        //         counter = 0;
        //     }
			
        //     // store both of those samples to output channels
        //     *(AUDIO_ptr + 2) = signal_val;
        //     *(AUDIO_ptr + 3) = signal_val;

        // }
    }
}

/*******************************************************************
 * Trap handler: determine what caused the interrupt and calls the
 * appropriate subroutine.
 ******************************************************************/
void handler(void) {
    int mcause_value;
    __asm__ volatile ("csrr %0, mcause" : "=r"(mcause_value));
    if (mcause_value == 0x80000010) // interval timer
        itimer_ISR();
    else if (mcause_value == 0x80000012) // KEY port
        KEY_ISR();
    else if (mcause_value == 0x80000016) // keyboard ps2
        PS2_ISR();
    else if (mcause_value == 0x80000015) // Audio FIFO // IRQ = 21
        audio_ISR();
    // else, ignore the trap
    else {
        printf("Unexpected interrupt at %d.", mcause_value);
    }
}

// FPGA interval timer interrupt service routine
void itimer_ISR(void) {
    *TIMER_ptr = 0; // clear interrupt
    sec_time -= 1;  // decrease pom timer counter
    if (sec_time==0) {  // if pom timer done, on 'stop' mode
        key_mode = 3;
        *(TIMER_ptr + 0x1) = 0xB;   // 0b1011 (stop, cont, ito)
    }
}

// KEY port interrupt service routine
void KEY_ISR(void) {
    int pressed_key;
    pressed_key = *(KEY_ptr + 3); // read EdgeCapture & get key value
    *(KEY_ptr + 3) = pressed_key; // clear EdgeCapture register
    if (pressed_key==1) {   // user presses start/pause/stop
        if (key_mode==1) {  // start
            *(TIMER_ptr + 0x1) = 0x7;   // 0b0111 (start, cont, ito)
            key_mode = 2;
        } else if (key_mode==2) {   // pause
            *(TIMER_ptr + 0x1) = 0xB;   // 0b1011 (stop, cont, ito)
            key_mode = 1;
        } else if (key_mode==3) {   // update next countdown start value
            study_mode = !study_mode;
            study_session_count++;
            if (study_mode) {
                sec_time = pom_start_val;
            } else if (!study_mode && study_session_count%4!=0) {
                sec_time = small_break_start_val;
            } else if (!study_mode) {
                sec_time = big_break_start_val;
            } else {
                printf("Unexpected study mode %d.", study_mode);
            }
            key_mode = 1;
        } else {
            printf("Unexpected key mode %d.", key_mode);
        }
    } else if (pressed_key==2) {

    } else {
        printf("Unexpected key %d pressed.", pressed_key);
    }
}

// Configure the FPGA interval timer
void set_itimer(void) {
    volatile int *TIMER_ptr = (int *) TIMER_BASE;
    // set the interval timer period
    int load_val = clock_rate;
    *(TIMER_ptr + 0x2) = (load_val & 0xFFFF);
    *(TIMER_ptr + 0x3) = (load_val >> 16) & 0xFFFF;
    *(TIMER_ptr + 0x0) = 0;
    // turn on CONT & ITO bits (do not start)
    *(TIMER_ptr + 0x1) = 0x3; // STOP = 1, START = 1, CONT = 1, ITO = 1
}

// Configure the KEY port
void set_KEY(void) {
    volatile int *KEY_ptr = (int *) KEY_BASE;
    *(KEY_ptr + 3) = 0xF; // clear EdgeCapture register
    *(KEY_ptr + 2) = 0x3; // enable interrupts for keys 0-1
}

void clear_screen() {
    int y, x;
    for (x = 0; x < 320; x++)
    for (y = 0; y < 240; y++)
    plot_pixel (x, y, 0);
}

void plot_pixel(int x, int y, short int line_color) {
    volatile short int *pixel_address;
    int offset = (y << 10) + (x << 1);
    *(volatile short int*)(pixel_buffer_start + offset) = line_color;
}

void display_num(int x, int y, short int line_color, int num) {
    if (num!=1 && num!=4) { // seg 0: 0 2 3 5 6 7 8 9
        for (int r=y+2; r<y+4; r++)
        for (int c=x+4; c<x+18; c++)
        plot_pixel(c, r, line_color);
    }
    if (num!=5 && num!=6) { // seg 1: 0 1 2 3 4 7 8 9
        for (int r=y+4; r<y+19; r++)
        for (int c=x+18; c<x+20; c++)
        plot_pixel(c, r, line_color);
    }
    if (num!=2) { // seg 2: 0 1 3 4 5 6 7 8 9
        for (int r=y+21; r<y+36; r++)
        for (int c=x+18; c<x+20; c++)
        plot_pixel(c, r, line_color);
    }
    if (num!=1 && num!=4 && num!=7) { // seg 3: 0 2 3 5 6 8 9
        for (int r=y+36; r<y+38; r++)
        for (int c=x+4; c<x+18; c++)
        plot_pixel(c, r, line_color);
    }
    if (num==0 || num==2 || num==6 || num==8) { // seg 4: 0 2 6 8
        for (int r=y+21; r<y+36; r++)
        for (int c=x+2; c<x+4; c++)
        plot_pixel(c, r, line_color);
    }
    if (num!=1 && num!=2 && num!=3 && num!=7) { // seg 5: 0 4 5 6 8 9
        for (int r=y+4; r<y+19; r++)
        for (int c=x+2; c<x+4; c++)
        plot_pixel(c, r, line_color);
    }
    if (num!=0 && num!=1 && num!=7) { // seg 6: 2 3 4 5 6 8 9
        for (int r=y+19; r<y+21; r++)
        for (int c=x+4; c<x+18; c++)
        plot_pixel(c, r, line_color);
    }
}

void countdown_display(int hex_value, int digits[]) {
    if (hex_value>59) {
        printf("Invalid value: Maximum allowed is 59\n");
        return;
    }
    
    int decimal_value = hex_value; // Convert from hex to decimal
    digits[1] = decimal_value / 10; // Extract tens digit
    digits[0] = decimal_value % 10; // Extract ones digit
}

void wait_for_v_sync() {
    volatile int* fbuf= (int*) 0xFF203020;
    int status;
    *fbuf = 1;
    status = *(fbuf + 3);
    while ((status & 0x01) != 0) {
        status = *(fbuf + 3); 
    }
}

// Configure the PS2 port for keyboard
void set_PS2()
{
    // Keep reading while RVALID (bit 15) is set
    while (*PS2_ptr & 0x8000)
    {
        PS2_data = *PS2_ptr; // Read and discard
    }

    *(PS2_ptr + 1) = 1; // enable interrupts RE bit
}

// KEY port interrupt service routine
void PS2_ISR(void) { // IRQ = 22
    PS2_data = *PS2_ptr; // Read data and implicitly decrement RAVAIL

    // Update byte history
    byte1 = byte2;
    byte2 = byte3;
    byte3 = PS2_data & 0xFF;
    
    
    if (byte2 == 0xF0)
    {
        if (byte1 == 0xE0) {
            switch (byte3)
            {
            // arrow keys
            case 0x75:
                led_display_val = 16;
                break; // Up
            case 0x6B:
                led_display_val = 32;
                break; // Left
            case 0x72:
                led_display_val = 64;
                break; // Down
            case 0x74:
                led_display_val = 128;
                break; // Right
            }
        }
        else {
            switch (byte3)
            {
            // numbers 0-9
            //0x45, 0x16, 0x1E, 0x26, 0x25, 0x2E, 0x36, 0x3D, 0x3E, 0x46
            case 0x45:
                led_display_val = 0;
                break; // 0
            case 0x16:
                led_display_val = 1;
                break; // 1
            case 0x1E:
                led_display_val = 2;
                break; // 2
            case 0x26:
                led_display_val = 3;
                break; // 3
            case 0x25:
                led_display_val = 4;
                break; // 4
            case 0x2E:
                led_display_val = 5;
                break; // 5
            case 0x36:
                led_display_val = 6;
                break; // 6
            case 0x3D:
                led_display_val = 7;
                break; // 7
            case 0x3E:
                led_display_val = 8;
                break; // 8
            case 0x46:
                led_display_val = 9;
                break; // 9

            // letters
            case 0x2D: // right now deosnt care if it is currently alarm or not
                led_display_val = 256;
                recording = false;
                break; // R
            case 0x4D: // right now deosnt care if it is currently alarm or not
                led_display_val = 256;
                void play_alarm(void);
                break; // R

            

            // function keys
            // 0x05, 0x06, 0x04 
            case 0x05:
                led_display_val = 256;
                break; // F1
            case 0x06:
                led_display_val = 256;
                break; // F2
            case 0x04:
                led_display_val = 256;
                break; // F3


            //other // enter, space, backspace
            //0x5A, 0x29, 0x66
            case 0x5A:
                led_display_val = 0;
                pressed_enter();
                break; // enter
            case 0x29:
                led_display_val = 256;
                break; // space
            case 0x66:
                led_display_val = 256;
                break; // backspace
            }
        }
    }
    else {
        if (byte3 == 0x2D) {
            led_display_val = 256;
            recording = true;
        }
    }
}

void pressed_enter(void) {   // user presses start/pause/stop
    // mode 1= currently paused
    // mode 2= currently counting
    // mode 3= 00, currently ringing

    if (key_mode==1) {  // start the timer countdown
        *(TIMER_ptr + 0x1) = 0x7;   // 0b0111 (start, cont, ito)
        key_mode = 2;
    } else if (key_mode==2) {   // pause the timer countdown
        *(TIMER_ptr + 0x1) = 0xB;   // 0b1011 (stop, cont, ito)
        key_mode = 1;
    } else if (key_mode==3) {   // stop the ringing update next countdown start value
        study_mode = !study_mode;
        study_session_count++;
        if (study_mode) {
            sec_time = pom_start_val;
        } else if (!study_mode && study_session_count%4!=0) {
            sec_time = small_break_start_val;
        } else if (!study_mode) {
            sec_time = big_break_start_val;
        } else {
            printf("Unexpected study mode %d.", study_mode);
        }
        key_mode = 1;
    } else {
        printf("Unexpected key mode %d.", key_mode);
    }
}



void set_AUDIO(void) {
    *(AUDIO_ptr) = 0b1111; // Clear FIFOs and enable write interrupts (WE)
}

volatile int audio_index = 0; // Current index in the audio buffer


void audio_ISR(void) {
    // Check if the write FIFO has space
    led_display_val = 0xF;
    fifospace = *(AUDIO_ptr + 1); // Read the audio FIFO space register
    if ((fifospace & 0x00FF) > 0) { // Check WSRC (write FIFO has space)
        // Calculate the number of samples per half-period dynamically
        int numSamples = (int)(1.0 / (2 * frequency * 0.000125)); // 0.000125 = 1/8kHz

        // Write the current signal value to the audio channels
        *(AUDIO_ptr + 2) = signal_val; // Left channel
        *(AUDIO_ptr + 3) = signal_val; // Right channel

        counter++;

        // Toggle the signal value every half-period
        if (counter >= numSamples) {
            signal_val *= -1;
            counter = 0; // Reset the counter
        }
    }
}

void play_alarm(void) {
    int i;
    for (i = 0; i < 100000; i++) {
        *(AUDIO_ptr + 2) = alarm_audio_left[i];
        *(AUDIO_ptr + 3) = alarm_audio_right[i];
    }
}