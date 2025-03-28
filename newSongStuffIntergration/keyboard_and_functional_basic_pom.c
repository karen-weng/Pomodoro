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
#define FPGA_CHAR_BASE			0x09000000
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

// keyboard functions
void pressed_enter(void);
void pressed_tab(void);
void pressed_up(void);
void pressed_down(void);

// ps2 variables
volatile int *PS2_ptr = (int *)0xFF200100; // PS/2 port address
volatile unsigned char PS2_data;
volatile unsigned char RVALID;
volatile int led_display_val = 0;
volatile unsigned char byte1 = 0;
volatile unsigned char byte2 = 0;
volatile unsigned char byte3 = 0;



// pom timer variables
volatile int pom_start_val = 25;
volatile int small_break_start_val = 5;
volatile int big_break_start_val = 15;
volatile int sec_time = 0;
volatile int min_time = 0;
volatile int key_mode = 1;  // 1 for start, 2 for pause, 3 for stop (ringing) --> multiply by 2 for break vals
volatile bool study_mode = true; // true for pomodoro, false for break,
volatile int study_session_count = 1;

volatile int *LEDR_ptr = (int *) LEDR_BASE;
volatile int *HEX3_HEX0_ptr = (int *) HEX3_HEX0_BASE;
volatile int *TIMER_ptr = (int *) TIMER_BASE;
volatile int *KEY_ptr = (int *) KEY_BASE;

void plot_pixel(int, int, short int); // plots one pixel
void clear_screen();    // clear whole screen
void clear_rectangle(int []);
void display_num(int, int, short int, int);
void hex_to_dec(int, int*);
void draw_line(int, int, int, int, short int);
void draw_rectangle(int[], short int);
void wait_for_v_sync();

volatile int* PIXEL_BUF_CTRL_ptr = (int*) PIXEL_BUF_CTRL_BASE;
int pixel_buffer_start; // global variable
short int buffer1[240][512]; // 240 rows, 512 (320 + padding) columns
short int buffer2[240][512];
short int white = 0xFFFFFF;
short int red = 0xB85C5C;

int num_w = 30;
int num_l = 40;
int loading1[] = {100, 130, 220, 150};
int loading2[] = {101, 131, 219, 149};
int area_to_erase[] = {100, 74, 220, 150};  // UPDATE THIS IF TWEAKING DISPLAY LOCATION
int dot1[] = {159, 90, 160, 91};
int dot2[] = {159, 100, 160, 101};

int main(void) {
    /* Declare volatile pointers to I/O registers (volatile means that the
     * accesses will always go to the memory (I/O) address */

    set_itimer();
    set_KEY();
    set_PS2();

    int mstatus_value, mtvec_value, mie_value;
    mstatus_value = 0b1000; // interrupt bit mask
    // disable interrupts
    __asm__ volatile ("csrc mstatus, %0" :: "r"(mstatus_value));
    mtvec_value = (int) &handler; // set trap address
    __asm__ volatile ("csrw mtvec, %0" :: "r"(mtvec_value));
    // disable all interrupts that are currently enabled
    __asm__ volatile ("csrr %0, mie" : "=r"(mie_value));
    __asm__ volatile ("csrc mie, %0" :: "r"(mie_value));
    mie_value = 0x450000; // KEY, itimer, ps/2 port (non-dual) -- irq 16, 18, 22
    // set interrupt enables
    __asm__ volatile ("csrs mie, %0" :: "r"(mie_value));
    // enable Nios V interrupts
    __asm__ volatile ("csrs mstatus, %0" :: "r"(mstatus_value));

    min_time = pom_start_val;

    /* set front pixel buffer to buffer 1 */
    *(PIXEL_BUF_CTRL_ptr + 1) = (int) &buffer1; // first store the address in the  back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_v_sync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *PIXEL_BUF_CTRL_ptr;
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    /* set back pixel buffer to buffer 2 */
    *(PIXEL_BUF_CTRL_ptr + 1) = (int) &buffer2;
    pixel_buffer_start = *(PIXEL_BUF_CTRL_ptr + 1); // we draw on the back buffer
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    int min_digits [2];
    int sec_digits [2];
    while (1) {
        hex_to_dec(min_time, min_digits);
        hex_to_dec(sec_time, sec_digits);
        wait_for_v_sync();
        pixel_buffer_start = *(PIXEL_BUF_CTRL_ptr + 1); // new back buffer
        clear_rectangle(area_to_erase);
        //clear_rectangle(loading1);
        draw_rectangle(loading1, white);  // display loading bar;
        draw_rectangle(loading2, white);  // display loading bar;
        for (int i=loading2[1]; i<loading2[3]; i++) {
            int tot;
            if (study_mode) {
                tot = pom_start_val*60;
            } else if (study_session_count%4!=0) {
                tot = small_break_start_val*60;
            } else {
                tot = big_break_start_val*60;
            }
            int num = (loading2[2]-loading2[0])*(tot-min_time*60-sec_time)/tot+loading2[0];
            draw_line(loading2[0], i, num, i, white);
        }
        display_num(loading1[0], loading1[1]-num_l*1.4, white, min_digits[1]);
        display_num(loading1[0]+num_w, loading1[1]-num_l*1.4, white, min_digits[0]);
        draw_rectangle(dot1, white);
        draw_rectangle(dot2, white);
        display_num(loading1[2]-num_w*2, loading1[1]-num_l*1.4, white, sec_digits[1]);
        display_num(loading1[2]-num_w, loading1[1]-num_l*1.4, white, sec_digits[0]);
        *LEDR_ptr = led_display_val;
    }
}

void wait_for_v_sync() {
    volatile int* fbuf= (int*) PIXEL_BUF_CTRL_BASE;
    *fbuf = 1;
    int status = *(fbuf + 3);
    while ((status & 0x01) != 0) {
        status = *(fbuf + 3); 
    }
}

void plot_pixel(int x, int y, short int line_color) {
    int offset = (y << 10) + (x << 1);
    *(volatile short int*)(pixel_buffer_start + offset) = line_color;
}

void draw_line(int x0, int y0, int x1,int y1, short int colour) {
    if (x0==x1) {
        for (int r=y0; r<=y1; r++)
        plot_pixel(x0, r, colour);
    } else if (y0==y1) {
        for (int c=x0; c<=x1; c++)
        plot_pixel(c, y0, colour);
    } else {
        // diagonal line
    }
}

void draw_rectangle(int coords[], short int colour) {   // empty rectangle (not filled)
    draw_line(coords[0], coords[1], coords[2], coords[1], colour);  // display loading bar
    draw_line(coords[0], coords[3], coords[2], coords[3], colour);
    draw_line(coords[0], coords[1], coords[0], coords[3], colour);
    draw_line(coords[2], coords[1], coords[2], coords[3], colour);
}

void clear_screen() {
    int coords[] = {0, 320, 0, 240};
    clear_rectangle(coords);
}

void clear_rectangle(int coords[]) {
    for (int x = coords[0]; x < coords[2]; x++)
    for (int y = coords[1]; y < coords[3]; y++)
    plot_pixel (x, y, 0);
}

void display_num(int x, int y, short int line_color, int num) {
    if (num!=1 && num!=4) { // seg 0: 0 2 3 5 6 7 8 9
        for (int r=y+2; r<y+4; r++)
        for (int c=x+8; c<x+num_w-8; c++)
        plot_pixel(c, r, line_color);
    }
    if (num!=5 && num!=6) { // seg 1: 0 1 2 3 4 7 8 9
        for (int r=y+4; r<y+19; r++)
        for (int c=x+num_w-8; c<x+num_w-6; c++)
        plot_pixel(c, r, line_color);
    }
    if (num!=2) { // seg 2: 0 1 3 4 5 6 7 8 9
        for (int r=y+21; r<y+num_l-4; r++)
        for (int c=x+num_w-8; c<x+num_w-6; c++)
        plot_pixel(c, r, line_color);
    }
    if (num!=1 && num!=4 && num!=7) { // seg 3: 0 2 3 5 6 8 9
        for (int r=y+36; r<y+num_l-2; r++)
        for (int c=x+8; c<x+num_w-8; c++)
        plot_pixel(c, r, line_color);
    }
    if (num==0 || num==2 || num==6 || num==8) { // seg 4: 0 2 6 8
        for (int r=y+21; r<y+num_l-4; r++)
        for (int c=x+6; c<x+8; c++)
        plot_pixel(c, r, line_color);
    }
    if (num!=1 && num!=2 && num!=3 && num!=7) { // seg 5: 0 4 5 6 8 9
        for (int r=y+4; r<y+19; r++)
        for (int c=x+6; c<x+8; c++)
        plot_pixel(c, r, line_color);
    }
    if (num!=0 && num!=1 && num!=7) { // seg 6: 2 3 4 5 6 8 9
        for (int r=y+19; r<y+21; r++)
        for (int c=x+8; c<x+num_w-8; c++)
        plot_pixel(c, r, line_color);
    }
}

void hex_to_dec(int hex_value, int digits[]) {
    if (hex_value>59) {
        printf("Invalid value: Maximum allowed is 59\n");
        return;
    }
    
    int decimal_value = hex_value; // Convert from hex to decimal
    digits[1] = decimal_value / 10; // Extract tens digit
    digits[0] = decimal_value % 10; // Extract ones digit
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
    // else, ignore the trap
    else {
        printf("Unexpected interrupt at %d.", mcause_value);
    }
}

// FPGA interval timer interrupt service routine
void itimer_ISR(void) {
    *TIMER_ptr = 0; // clear interrupt
    if (sec_time==0 && min_time!=0) {   // down a minute but timer not over
        min_time -= 1;
        sec_time = 59;
    } else {
        sec_time -= 1;  // decrease pom timer counter
    }
    if (sec_time==0 && min_time==0) {  // if pom timer done, on 'stop' mode
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
            if (study_mode) {
                min_time = pom_start_val;
                study_session_count++;
            } else if (!study_mode && study_session_count%4!=0) {
                min_time = small_break_start_val;
            } else if (!study_mode) {
                min_time = big_break_start_val;
            } else {
                printf("Unexpected study mode %d.", study_mode);
            }
            key_mode = 1;
        } else {
            printf("Unexpected key mode %d.", key_mode);
        }
    } else if (pressed_key==2) {    // skip
        *(TIMER_ptr + 0x1) = 0xB;
        key_mode = 1;   // auto-set to start
        study_mode = !study_mode;
        sec_time = 0;
        if (study_mode) {
            min_time = pom_start_val;
            study_session_count++;
        } else if (!study_mode && study_session_count%4!=0) {
            min_time = small_break_start_val;
        } else if (!study_mode) {
            min_time = big_break_start_val;
        } else {
            printf("Unexpected study mode %d.", study_mode);
        }
    // for increasing & decreasing start value, check mode & if haven't ever pressed start
    } else if (pressed_key==4 && key_mode==1) { // increase start value
        if (study_mode && min_time==pom_start_val) {
            pom_start_val++;
            min_time = pom_start_val;
        } else if (!study_mode && min_time==small_break_start_val && study_session_count%4!=0) {
            small_break_start_val++;
            min_time = small_break_start_val;
        } else if (!study_mode && min_time==big_break_start_val) {
            big_break_start_val++;
            min_time = big_break_start_val;
        }   // else user already started timer once, needs to skip/finish current session
    } else if (pressed_key==8 && key_mode==1) { // decrease start value
        if (study_mode && min_time==pom_start_val) {
            pom_start_val--;
            min_time = pom_start_val;       
        } else if (!study_mode && min_time==small_break_start_val && study_session_count%4!=0) {
            small_break_start_val--;
            min_time = small_break_start_val;
        } else if (!study_mode && min_time==big_break_start_val) {
            big_break_start_val--;
            min_time = big_break_start_val;
        }
    } else {
        printf("Unexpected key %d pressed.", pressed_key);
    }
}

// KEY port interrupt service routine
void PS2_ISR(void) { // IRQ = 22
    led_display_val = 0;
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
                pressed_up();
                break; // Up
            case 0x6B:
                led_display_val = 32;
                break; // Left
            case 0x72:
                led_display_val = 64;
                pressed_down();
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
                // recording = false;
                break; // R
            case 0x4D: // right now deosnt care if it is currently alarm or not
                led_display_val = 256;
                // play_alarm(void);
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


            //other // enter, tab, space, backspace
            //0x5A, 0x0D, 0x29, 0x66
            case 0x5A:
                led_display_val = 512;
                pressed_enter();
                break; // enter
            case 0x0D:
                led_display_val = 512;
                pressed_tab();
                break; // tab
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
            // recording = true;
        }
    }
}

void pressed_enter(void) {   // user presses start/pause/stop
    // mode 1= currently paused
    // mode 2= currently counting
    // mode 3= 00, currently ringing

    if (key_mode==1) {  // start
        *(TIMER_ptr + 0x1) = 0x7;   // 0b0111 (start, cont, ito)
        key_mode = 2;
    } else if (key_mode==2) {   // pause
        *(TIMER_ptr + 0x1) = 0xB;   // 0b1011 (stop, cont, ito)
        key_mode = 1;
    } else if (key_mode==3) {   // update next countdown start value
        study_mode = !study_mode;
        if (study_mode) {
            min_time = pom_start_val;
            study_session_count++;
        } else if (!study_mode && study_session_count%4!=0) {
            min_time = small_break_start_val;
        } else if (!study_mode) {
            min_time = big_break_start_val;
        } else {
            printf("Unexpected study mode %d.", study_mode);
        }
        key_mode = 1;
    } else {
        printf("Unexpected key mode %d.", key_mode);
    }
}

void pressed_tab(void) { // skip
    *(TIMER_ptr + 0x1) = 0xB;
    key_mode = 1;   // auto-set to start
    study_mode = !study_mode;
    sec_time = 0;
    if (study_mode) {
        min_time = pom_start_val;
        study_session_count++;
    } else if (!study_mode && study_session_count%4!=0) {
        min_time = small_break_start_val;
    } else if (!study_mode) {
        min_time = big_break_start_val;
    } else {
        printf("Unexpected study mode %d.", study_mode);
    }
}

void pressed_up(void) { 
    if (key_mode==1) { // increase start value
        if (study_mode && min_time==pom_start_val) {
            pom_start_val++;
            min_time = pom_start_val;
        } else if (!study_mode && min_time==small_break_start_val && study_session_count%4!=0) {
            small_break_start_val++;
            min_time = small_break_start_val;
        } else if (!study_mode && min_time==big_break_start_val) {
            big_break_start_val++;
            min_time = big_break_start_val;
        }   // else user already started timer once, needs to skip/finish current session
    }
}

void pressed_down(void) { // skip
    if (key_mode==1) { // decrease start value
        if (study_mode && min_time==pom_start_val) {
            pom_start_val--;
            min_time = pom_start_val;       
        } else if (!study_mode && min_time==small_break_start_val && study_session_count%4!=0) {
            small_break_start_val--;
            min_time = small_break_start_val;
        } else if (!study_mode && min_time==big_break_start_val) {
            big_break_start_val--;
            min_time = big_break_start_val;
        }
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
    *(KEY_ptr + 2) = 0xF; // enable interrupts for keys 0-3
}

// Configure the PS2 port for keyboard
void set_PS2() {
    // Keep reading while RVALID (bit 15) is set
    while (*PS2_ptr & 0x8000) {
        PS2_data = *PS2_ptr; // Read and discard
    }
    *(PS2_ptr + 1) = 1; // enable interrupts RE bit
}
