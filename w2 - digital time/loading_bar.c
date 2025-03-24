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
void clear_screen();    // clear whole screen
void clear_rectangle(int []);
void display_num(int, int, short int, int);
void countdown_display(int, int*);
void draw_line(int, int, int, int, short int);
void draw_rectangle(int[], short int);
void wait_for_v_sync();

volatile int* PIXEL_BUF_CTRL_ptr = (int*) PIXEL_BUF_CTRL_BASE;
int pixel_buffer_start; // global variable
short int buffer1[240][512]; // 240 rows, 512 (320 + padding) columns
short int buffer2[240][512];
short int white = 0xFFFF;
int loading[] = {100, 150, 220, 170};
int num_coords[] = {132,80,200,120};

int main(void) {
    /* Declare volatile pointers to I/O registers (volatile means that the
     * accesses will always go to the memory (I/O) address */

    set_itimer();
    set_KEY();

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

    sec_time = pom_start_val;

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
    draw_rectangle(loading, white);  // display loading bar;
    int n = 0;
    int count = 0;
    int digits [2];
    while (1) {
        countdown_display(sec_time, digits);
        wait_for_v_sync();
        pixel_buffer_start = *(PIXEL_BUF_CTRL_ptr + 1); // new back buffer
        draw_rectangle(loading, white);  // display loading bar;
        for (int i=loading[1]; i<loading[3]; i++) {
            int num = (loading[2]-loading[0])*(pom_start_val-sec_time)/pom_start_val+loading[0];
            draw_line(loading[0], i, num, i, white);
        }
        clear_rectangle(num_coords);
        display_num(132, 80, white, digits[1]);
        display_num(164, 80, white, digits[0]);
        //display_loading(sec_time, pom_start_val);
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
            //clear_rectangle(loading);
            //draw_rectangle(loading, white);
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
    int coords[] = {0, 320, 0, 240};
    clear_rectangle(coords);
}

void clear_rectangle(int coords[]) {
    for (int x = coords[0]; x < coords[2]; x++)
    for (int y = coords[1]; y < coords[3]; y++)
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

void draw_line(int x0, int y0, int x1,int y1, short int colour) {
    if (x0==x1) {
        for (int r=y0; r<=y1; r++)
        plot_pixel(x0, r, colour);
    } else if (y0==y1) {
        for (int c=x0; c<=x1; c++)
        plot_pixel(c, y0, colour);
    } else {

    }
}

void draw_rectangle(int coords[], short int colour) {
    draw_line(coords[0], coords[1], coords[2], coords[1], colour);  // display loading bar
    draw_line(coords[0], coords[3], coords[2], coords[3], colour);
    draw_line(coords[0], coords[1], coords[0], coords[3], colour);
    draw_line(coords[2], coords[1], coords[2], coords[3], colour);
}

void wait_for_v_sync() {
    volatile int* fbuf= (int*) PIXEL_BUF_CTRL_BASE;
    *fbuf = 1;
    int status = *(fbuf + 3);
    while ((status & 0x01) != 0) {
        status = *(fbuf + 3); 
    }
}