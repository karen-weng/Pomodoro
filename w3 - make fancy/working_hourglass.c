#define BOARD "DE1-SoC"

/* Memory */
#define DDR_BASE 0x40000000
#define DDR_END 0x7FFFFFFF
#define A9_ONCHIP_BASE 0xFFFF0000
#define A9_ONCHIP_END 0xFFFFFFFF
#define SDRAM_BASE 0x00000000
#define SDRAM_END 0x03FFFFFF
#define FPGA_PIXEL_BUF_BASE 0x08000000
#define FPGA_PIXEL_BUF_END 0x0803FFFF
#define FPGA_CHAR_BASE 0x09000000
#define FPGA_CHAR_END 0x09001FFF

/* Cyclone V FPGA devices */
#define LED_BASE 0xFF200000
#define LEDR_BASE 0xFF200000
#define HEX3_HEX0_BASE 0xFF200020
#define HEX5_HEX4_BASE 0xFF200030
#define SW_BASE 0xFF200040
#define KEY_BASE 0xFF200050
#define JP1_BASE 0xFF200060
#define JP2_BASE 0xFF200070
#define PS2_BASE 0xFF200100
#define PS2_DUAL_BASE 0xFF200108
#define JTAG_UART_BASE 0xFF201000
#define IrDA_BASE 0xFF201020
#define TIMER_BASE 0xFF202000
#define TIMER_2_BASE 0xFF202020
#define AV_CONFIG_BASE 0xFF203000
#define RGB_RESAMPLER_BASE 0xFF203010
#define PIXEL_BUF_CTRL_BASE 0xFF203020
#define CHAR_BUF_CTRL_BASE 0xFF203030
#define AUDIO_BASE 0xFF203040
#define VIDEO_IN_BASE 0xFF203060
#define EDGE_DETECT_CTRL_BASE 0xFF203070
#define ADC_BASE 0xFF204000

/* Cyclone V HPS devices */
#define HPS_GPIO1_BASE 0xFF709000
#define I2C0_BASE 0xFFC04000
#define I2C1_BASE 0xFFC05000
#define I2C2_BASE 0xFFC06000
#define I2C3_BASE 0xFFC07000
#define HPS_TIMER0_BASE 0xFFC08000
#define HPS_TIMER1_BASE 0xFFC09000
#define HPS_TIMER2_BASE 0xFFD00000
#define HPS_TIMER3_BASE 0xFFD01000
#define FPGA_BRIDGE 0xFFD0501C

// #include "address_map_niosv.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// header file setup
// #include "samples_rooster_alarm.h"  // Include the header file with the list definition

#define clock_rate 100000000
#define quarter_clock clock_rate / 4

static void handler(void) __attribute__((interrupt("machine")));
void set_itimer(void);
void set_itimer_audio(void);
void set_KEY(void);
void itimer_ISR(void);
void audio_ISR_timer2(void);
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
volatile int tot;
volatile int min_time = 0;
volatile int key_mode = 1;       // 1 for start, 2 for pause, 3 for stop (ringing) --> multiply by 2 for break vals
volatile bool study_mode = true; // true for pomodoro, false for break,
volatile int study_session_count = 1;

volatile int *LEDR_ptr = (int *)LEDR_BASE;
volatile int *HEX3_HEX0_ptr = (int *)HEX3_HEX0_BASE;
volatile int *TIMER_ptr = (int *)TIMER_BASE;
volatile int *TIMER_AUDIO_ptr = (int *)TIMER_2_BASE;

volatile int *KEY_ptr = (int *)KEY_BASE;
// vga functions
void plot_pixel(int, int, short int); // plots one pixel
void clear_screen(short int);         // clear whole screen
void clear_rectangle(int[], short int);

// vga timer functions
void display_num(int, int, short int, int);
void erase_num(int, int);
void hex_to_dec(int, int *);
void draw_line(int, int, int, int, short int);
void draw_rectangle(int[], short int);
void wait_for_v_sync();

// vga hourglass
void draw_hourglass_frame();
// void draw_hourglass_frame_big();
void get_hourglass_bounds(int y, int *x_left, int *x_right);
void setup_hourglass();
void draw_hourglass_top(int top);
void draw_hourglass_bottom(int top);
void draw_hourglass_drip();

void toggle_display();

void reset_start_time(int start_time);

int hourglass_erase[] = {89, 0, 232, 191}; // UPDATE THIS IF TWEAKING DISPLAY LOCATION
// int hourglass_erase[] = {89, 50, 231, 191}; // UPDATE THIS IF TWEAKING DISPLAY LOCATION

int display_mode = 1; // 1 for loading bar, 2 for hourglass
int hourglass_draw_index = 0;
int hourglass_sec_counter = 0;
int hourglass_sec_to_wait = 0;
int hourglass_x_left;
int hourglass_x_right;
int hourglass_drip_end = 0; 
int hourglass_drip_start = 0; 
int drip_wait_counter = 0;
int drip_wait_time = 150;


volatile int *PIXEL_BUF_CTRL_ptr = (int *)PIXEL_BUF_CTRL_BASE;
int pixel_buffer_start;      // global variable
short int buffer1[240][512]; // 240 rows, 512 (320 + padding) columns
short int buffer2[240][512];
volatile int colour;
short int black = 0x0;
short int white = 0xFFFF;
short int grey = 0x8494;
short int red = 0xB2EB;
short int dark_red = 0xA489;
short int teal = 0x5B92;
short int dark_teal = 0x4310;
short int navy = 0x52F3;
short int dark_navy = 0x44D2;

int num_w = 30;
int num_l = 40;
int loading1[] = {100, 130, 220, 150};
int loading2[] = {101, 131, 219, 149};
int area_to_erase[] = {100, 74, 220, 150}; // UPDATE THIS IF TWEAKING DISPLAY LOCATION
int dot1[] = {159, 90, 160, 91};
int dot2[] = {159, 100, 160, 101};
int dot3[] = {159, 25, 160, 26};
int dot4[] = {159, 35, 160, 36};
int rooster_num_samples = 28961;
// rooster bottom

int background_sample_index = 0;
int pause_sample_index = 0;
int rooster_sample_index = 0;

double volume_factor = 0.5;
bool mute = false;

struct audio_t {
    volatile unsigned int control;
    volatile unsigned char rarc;
    volatile unsigned char ralc;
    volatile unsigned char warc;
    volatile unsigned char walc;
    volatile unsigned int ldata;
    volatile unsigned int rdata;
};

struct audio_t *const AUDIO_ptr = ((struct audio_t *)AUDIO_BASE);

int main(void) {
    /* Declare volatile pointers to I/O registers (volatile means that the
     * accesses will always go to the memory (I/O) address */

    set_itimer();
    set_itimer_audio();
    set_KEY();
    set_PS2();

    int mstatus_value, mtvec_value, mie_value;
    mstatus_value = 0b1000; // interrupt bit mask
    // disable interrupts
    __asm__ volatile("csrc mstatus, %0" ::"r"(mstatus_value));
    mtvec_value = (int)&handler; // set trap address
    __asm__ volatile("csrw mtvec, %0" ::"r"(mtvec_value));
    // disable all interrupts that are currently enabled
    __asm__ volatile("csrr %0, mie" : "=r"(mie_value));
    __asm__ volatile("csrc mie, %0" ::"r"(mie_value));
    mie_value = 0x470000; // KEY, itimer, itimer2 (audio), ps/2 port (non-dual) -- irq 16, 17, 18, 22
    // set interrupt enables
    __asm__ volatile("csrs mie, %0" ::"r"(mie_value));
    // enable Nios V interrupts
    __asm__ volatile("csrs mstatus, %0" ::"r"(mstatus_value));

    min_time = pom_start_val;
    colour = red;
    tot = min_time*60;
    hourglass_sec_to_wait = pom_start_val;

    /* set front pixel buffer to buffer 1 */
    *(PIXEL_BUF_CTRL_ptr + 1) = (int)&buffer1; // first store the address in the  back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_v_sync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *PIXEL_BUF_CTRL_ptr;
    clear_screen(colour); // pixel_buffer_start points to the pixel buffer
    draw_rectangle(dot1, white);
    draw_rectangle(dot2, white);
    /* set back pixel buffer to buffer 2 */
    *(PIXEL_BUF_CTRL_ptr + 1) = (int)&buffer2;
    pixel_buffer_start = *(PIXEL_BUF_CTRL_ptr + 1); // we draw on the back buffer
    clear_screen(colour); // pixel_buffer_start points to the pixel buffer
    draw_rectangle(dot1, white);
    draw_rectangle(dot2, white);
    int min_digits[2];
    int sec_digits[2];

    while (1) {
        hex_to_dec(min_time, min_digits);
        hex_to_dec(sec_time, sec_digits);
        wait_for_v_sync();
        pixel_buffer_start = *(PIXEL_BUF_CTRL_ptr + 1); // new back buffer

        *LEDR_ptr = display_mode;
        // *LEDR_ptr = hourglass_sec_to_wait;

        if (display_mode == 1) {
            clear_rectangle(hourglass_erase, colour); // clear hourglass form toggling
            // clear_rectangle(area_to_erase);
            // clear_rectangle(loading1);
            draw_rectangle(loading1, white); // display loading bar;
            draw_rectangle(loading2, white); // display loading bar;
            for (int i = loading2[1]; i < loading2[3]; i++)
            {
                if (study_mode) {
                    tot = pom_start_val * 60;
                } else if (study_session_count % 4 != 0) {
                    tot = small_break_start_val * 60;
                } else {
                    tot = big_break_start_val * 60;
                }
                int num = (loading2[2] - loading2[0]) * (tot - min_time * 60 - sec_time) / tot + loading2[0];
                draw_line(loading2[0], i, num, i, white);
            }
            display_num(loading1[0], loading1[1] - num_l * 1.4, white, min_digits[1]);
            display_num(loading1[0] + num_w, loading1[1] - num_l * 1.4, white, min_digits[0]);
            draw_rectangle(dot1, white);
            draw_rectangle(dot2, white);
            display_num(loading1[2] - num_w * 2, loading1[1] - num_l * 1.4, white, sec_digits[1]);
            display_num(loading1[2] - num_w, loading1[1] - num_l * 1.4, white, sec_digits[0]);
        }
        else if (display_mode == 2) {
            clear_rectangle(hourglass_erase, colour);
            draw_hourglass_top(61 + hourglass_draw_index);
            draw_hourglass_bottom(190 - hourglass_draw_index);
            draw_hourglass_drip();
            draw_hourglass_frame();
            display_num(loading1[0], loading1[1] - num_l * 3, white, min_digits[1]);
            display_num(loading1[0] + num_w, loading1[1] - num_l * 3, white, min_digits[0]);
            draw_rectangle(dot3, white);
            draw_rectangle(dot4, white);
            display_num(loading1[2] - num_w * 2, loading1[1] - num_l * 3, white, sec_digits[1]);
            display_num(loading1[2] - num_w, loading1[1] - num_l * 3, white, sec_digits[0]);
        }

    }
        
}

void wait_for_v_sync() {
    volatile int *fbuf = (int *)PIXEL_BUF_CTRL_BASE;
    *fbuf = 1;
    int status = *(fbuf + 3);
    while ((status & 0x01) != 0) {
        status = *(fbuf + 3);
    }
}

void plot_pixel(int x, int y, short int line_color) {
    int offset = (y << 10) + (x << 1);
    *(volatile short int *)(pixel_buffer_start + offset) = line_color;
}

void draw_line(int x0, int y0, int x1, int y1, short int colour) {
    if (x0 == x1) {
        for (int r = y0; r <= y1; r++)
            plot_pixel(x0, r, colour);
    }
    else if (y0 == y1) {
        for (int c = x0; c <= x1; c++)
            plot_pixel(c, y0, colour);
    }
    else {
        bool is_steep = abs(y1 - y0) > abs(x1 - x0);
        if (is_steep) {
            int temp = x0;
            x0 = y0;
            y0 = temp;
            temp = x1;
            x1 = y1;
            y1 = temp;
        }
        if (x0 > x1) {
            int temp = x0;
            x0 = x1;
            x1 = temp;
            temp = y0;
            y0 = y1;
            y1 = temp;
        }
        int deltax = x1 - x0, deltay = abs(y1 - y0), error = -(deltax / 2), y = y0;
        int y_step = (y0 < y1) ? 1 : -1;
        for (int x = x0; x <= x1; x++) {
            if (is_steep)   // no curly braces makes this confusing to understand
                plot_pixel(y, x, colour);
            else
                plot_pixel(x, y, colour);
            error += deltay;    // idk if this is part of the else
            if (error > 0) {
                y += y_step;
                error -= deltax;
            }
        }
    }
}

void draw_rectangle(int coords[], short int colour) {                                                                  // empty rectangle (not filled)
    draw_line(coords[0], coords[1], coords[2], coords[1], colour); // display loading bar
    draw_line(coords[0], coords[3], coords[2], coords[3], colour);
    draw_line(coords[0], coords[1], coords[0], coords[3], colour);
    draw_line(coords[2], coords[1], coords[2], coords[3], colour);
}

void clear_screen(short int c) {
    int coords[] = {0, 0, 320, 240};
    clear_rectangle(coords, c);
}

void clear_rectangle(int coords[], short int c) {
    for (int x = coords[0]; x < coords[2]; x++)
        for (int y = coords[1]; y < coords[3]; y++)
            plot_pixel(x, y, c);
}

void erase_num(int x, int y) {
    // erase
    for (int r=y+2; r<y+4; r++)         // seg 0
    for (int c=x+8; c<x+num_w-8; c++)
        plot_pixel(c, r, colour);
    for (int r=y+4; r<y+19; r++)        // seg 1
    for (int c=x+num_w-8; c<x+num_w-6; c++)
        plot_pixel(c, r, colour);
    for (int r=y+21; r<y+num_l-4; r++)  // seg 2
    for (int c=x+num_w-8; c<x+num_w-6; c++)
        plot_pixel(c, r, colour);
    for (int r=y+36; r<y+num_l-2; r++)  // seg 3
    for (int c=x+8; c<x+num_w-8; c++)
        plot_pixel(c, r, colour);
    for (int r=y+21; r<y+num_l-4; r++)  // seg 4
    for (int c=x+6; c<x+8; c++)
        plot_pixel(c, r, colour);
    for (int r=y+4; r<y+19; r++)        // seg 5
    for (int c=x+6; c<x+8; c++)
        plot_pixel(c, r, colour);
    for (int r=y+19; r<y+21; r++)       // seg 6
    for (int c=x+8; c<x+num_w-8; c++)
        plot_pixel(c, r, colour);
}

void display_num(int x, int y, short int line_color, int num) {
    erase_num(x, y);
    // draw
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
    if (hex_value > 59) {
        printf("Invalid value: Maximum allowed is 59\n");
        return;
    }

    int decimal_value = hex_value;  // Convert from hex to decimal
    digits[1] = decimal_value / 10; // Extract tens digit
    digits[0] = decimal_value % 10; // Extract ones digit
}

/*******************************************************************
 * Trap handler: determine what caused the interrupt and calls the
 * appropriate subroutine.
 ******************************************************************/
void handler(void)
{
    int mcause_value;
    __asm__ volatile("csrr %0, mcause" : "=r"(mcause_value));
    if (mcause_value == 0x80000010) // interval timer
        itimer_ISR();
    else if (mcause_value == 0x80000011)
    { // interval timer 2 for audio
        led_display_val = 1;
        audio_ISR_timer2();
    }
    else if (mcause_value == 0x80000012) // KEY port
        KEY_ISR();
    else if (mcause_value == 0x80000016) // keyboard ps2
        PS2_ISR();
    // else, ignore the trap
    else
    {
        printf("Unexpected interrupt at %d.", mcause_value);
    }
}

// FPGA interval timer interrupt service routine
void itimer_ISR(void) {
    *TIMER_ptr = 0; // clear interrupt
    if (sec_time == 0 && min_time != 0)
    { // down a minute but timer not over
        min_time -= 1;
        sec_time = 59;
    }
    else {
        sec_time -= 1; // decrease pom timer counter
    }
    if (sec_time == 0 && min_time == 0)
    { // if pom timer done, on 'stop' mode
        key_mode = 3;
        *(TIMER_ptr + 0x1) = 0xB; // 0b1011 (stop, cont, ito)
    }

    if (hourglass_sec_counter < hourglass_sec_to_wait-1) {
        hourglass_sec_counter++;
    }
    else {
        hourglass_sec_counter = 0;
        hourglass_draw_index++;
        // hourglass_new_segment = true;
    }
    
}

void play_audio_samples(int *samples, int samples_n, int *sample_index)
{
    if (*sample_index < samples_n)
    {
        if (AUDIO_ptr->warc > 0)
        {
            int scaled_sample = (int)(samples[*sample_index] * volume_factor);

            AUDIO_ptr->ldata = scaled_sample;
            AUDIO_ptr->rdata = scaled_sample;

            (*sample_index)++;
        }
    }
    else
    {
        *sample_index = 0; // Loop back if end is reached
    }
}

// audio ISR, techcially timer
void audio_ISR_timer2(void){
    *TIMER_AUDIO_ptr = 0; // reset interrupt
    // already set to continue timer

    // TODO add mute variable

    
    // if (!mute) {
    //     if (key_mode == 2)
    //     {
    //         if (study_mode == true)
    //         { // studying
    //             // play_audio_samples(background_samples, background_num_samples, &background_sample_index);

    //         } // if counting
    //         else
    //         {
    //             // play_audio_samples(pause_samples, pause_num_samples, &pause_sample_index);
    //         }
    //     }
    //     else if (key_mode == 1)
    //     {
    //         // play_audio_samples(pause_samples, pause_num_samples, &pause_sample_index);
    //     }
    //     else if (key_mode == 3) {
    //         // play_audio_samples(rooster_samples, rooster_num_samples, &rooster_sample_index);
    //     }
    // }

    // ^ THAT IS IMPORTANT IT IS AUDIO ITS JUST COMPONENTED OUT CAUSE ITS SLOW

    if (key_mode == 2) { // if counting
        if (drip_wait_counter < drip_wait_time) {
            drip_wait_counter++;
        }
        else {
            drip_wait_counter = 0;
            if ((hourglass_drip_end < 70) && (hourglass_draw_index <= 1)) {
                hourglass_drip_end++;
            }
            else if ((hourglass_draw_index >= 59) && (hourglass_drip_start < 70)) {
                hourglass_drip_start++;
            }
        }
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
                colour = red;
            } else if (!study_mode && study_session_count%4!=0) {
                min_time = small_break_start_val;
                colour = teal;
            } else if (!study_mode) {
                min_time = big_break_start_val;
                colour = navy;
            } else {
                printf("Unexpected study mode %d.", study_mode);
            }
            // clear_screen(colour);
            // draw_rectangle(dot1, white);
            // draw_rectangle(dot2, white);
            // wait_for_v_sync();
            // pixel_buffer_start = *(PIXEL_BUF_CTRL_ptr + 1); // new back buffer
            // clear_screen(colour);
            // draw_rectangle(dot1, white);
            // draw_rectangle(dot2, white);
            // wait_for_v_sync();
            // pixel_buffer_start = *(PIXEL_BUF_CTRL_ptr + 1); // new back buffer
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
            colour = red;
        } else if (!study_mode && study_session_count%4!=0) {
            min_time = small_break_start_val;
            colour = teal;
        } else if (!study_mode) {
            min_time = big_break_start_val;
            colour = navy;
        } else {
            printf("Unexpected study mode %d.", study_mode);
        }
        // clear_screen(black);
        // draw_rectangle(dot1, white);
        // draw_rectangle(dot2, white);
        // wait_for_v_sync();
        // pixel_buffer_start = *(PIXEL_BUF_CTRL_ptr + 1); // new back buffer
        // clear_screen(black);
        // draw_rectangle(dot1, white);
        // draw_rectangle(dot2, white);
        // wait_for_v_sync();
        // pixel_buffer_start = *(PIXEL_BUF_CTRL_ptr + 1); // new back buffer
    // for increasing & decreasing start value, check mode & if haven't ever pressed start
    } else if (pressed_key==4 && key_mode==1 && min_time<99) { // increase start value
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
    } else if (pressed_key==8 && key_mode==1 && min_time>1) { // decrease start value
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
void PS2_ISR(void)
{ // IRQ = 22
    // led_display_val = 0;
    PS2_data = *PS2_ptr; // Read data and implicitly decrement RAVAIL

    // Update byte history
    byte1 = byte2;
    byte2 = byte3;
    byte3 = PS2_data & 0xFF;

    if (byte2 == 0xF0)
    {
        if (byte1 == 0xE0)
        {
            switch (byte3)
            {
            // arrow keys
            case 0x75:
                // led_display_val = 16;
                pressed_up();
                break; // Up
            case 0x6B:
                // led_display_val = 32;
                toggle_display();
                break; // Left
            case 0x72:
                // led_display_val = 64;
                pressed_down();
                break; // Down
            case 0x74:
                // led_display_val = 128;
                toggle_display();
                break; // Right
            }
        }
        else
        {
            switch (byte3)
            {
            // numbers 0-9
            // 0x45, 0x16, 0x1E, 0x26, 0x25, 0x2E, 0x36, 0x3D, 0x3E, 0x46
            case 0x45:
                // led_display_val = 0;
                break; // 0
            case 0x16:
                // led_display_val = 1;
                break; // 1
            case 0x1E:
                // led_display_val = 2;
                break; // 2
            case 0x26:
                // led_display_val = 3;
                break; // 3
            case 0x25:
                // led_display_val = 4;
                break; // 4
            case 0x2E:
                // led_display_val = 5;
                break; // 5
            case 0x36:
                // led_display_val = 6;
                break; // 6
            case 0x3D:
                // led_display_val = 7;
                break; // 7
            case 0x3E:
                // led_display_val = 8;
                break; // 8
            case 0x46:
                // led_display_val = 9;
                break; // 9

            // letters
            case 0x2D: // right now deosnt care if it is currently alarm or not
                // led_display_val = 256;
                // recording = false;
                break; // R
            case 0x4D: // right now deosnt care if it is currently alarm or not
                // led_display_val = 256;
                // play_alarm(void);
                break; // R

            // function keys
            // 0x05, 0x06, 0x04
            case 0x05:
                // led_display_val = 256;
                mute = !mute;
                break; // F1 // mute
            case 0x06:
                // led_display_val = 256;
                if (volume_factor > 0.1) {
                    volume_factor -= 0.1;
                }
                break; // F2 // decrease volume
            case 0x04:
                // led_display_val = 256;
                if (volume_factor < 0.9) {
                    volume_factor += 0.1;
                }
                break; // F3 // increase volume

            // other // enter, tab, space, backspace
            // 0x5A, 0x0D, 0x29, 0x66
            case 0x5A:
                // led_display_val = 512;
                pressed_enter();
                break; // enter
            case 0x0D:
                // led_display_val = 512;
                pressed_tab();
                break; // tab
            case 0x29:
                // led_display_val = 256;
                break; // space
            case 0x66:
                // led_display_val = 256;
                break; // backspace
            }
        }
    }
    else
    {
        if (byte3 == 0x2D)
        {
            // led_display_val = 256;
            // recording = true;
        }
    }
}

void pressed_enter(void)
{ // user presses start/pause/stop
    // mode 1= currently paused
    // mode 2= currently counting
    // mode 3= 00, currently ringing

    if (key_mode == 1)
    {                             // start
        *(TIMER_ptr + 0x1) = 0x7; // 0b0111 (start, cont, ito)
        // *(TIMER_AUDIO_ptr + 0x1) = 0x7;
        key_mode = 2;
    }
    else if (key_mode == 2)
    {                             // pause
        *(TIMER_ptr + 0x1) = 0xB; // 0b1011 (stop, cont, ito)
        key_mode = 1;
    }
    else if (key_mode == 3)
    { // update next countdown start value
        study_mode = !study_mode;
        if (study_mode)
        {
            reset_start_time(pom_start_val);
            
            study_session_count++;
        }
        else if (!study_mode && study_session_count % 4 != 0)
        {
            reset_start_time(small_break_start_val);

        }
        else if (!study_mode)
        {
            reset_start_time(big_break_start_val);

        }
        else
        {
            printf("Unexpected study mode %d.", study_mode);
        }
        key_mode = 1;
    }
    else
    {
        printf("Unexpected key mode %d.", key_mode);
    }
}

void pressed_tab(void)
{ // skip
    *(TIMER_ptr + 0x1) = 0xB;
    key_mode = 1; // auto-set to start
    study_mode = !study_mode;
    sec_time = 0;
    if (study_mode)
    {
        reset_start_time(pom_start_val);
        
        study_session_count++;
    }
    else if (!study_mode && study_session_count % 4 != 0)
    {
        reset_start_time(small_break_start_val);

    }
    else if (!study_mode)
    {
        reset_start_time(big_break_start_val);

    }
    else
    {
        printf("Unexpected study mode %d.", study_mode);
    }
}

void pressed_up(void)
{
    if (key_mode == 1 && min_time<99)
    { // increase start value
        if (study_mode && min_time == pom_start_val)
        {
            pom_start_val++;
            reset_start_time(pom_start_val);
        }
        else if (!study_mode && min_time == small_break_start_val && study_session_count % 4 != 0)
        {
            small_break_start_val++;
            reset_start_time(small_break_start_val);
        }
        else if (!study_mode && min_time == big_break_start_val)
        {
            big_break_start_val++;
            reset_start_time(big_break_start_val);
        } // else user already started timer once, needs to skip/finish current session
    }
}

void pressed_down(void)
{ // skip
    if (key_mode == 1 && min_time>1)
    { // decrease start value
        if (study_mode && min_time == pom_start_val)
        {
            pom_start_val--;
            reset_start_time(pom_start_val);
        }
        else if (!study_mode && min_time == small_break_start_val && study_session_count % 4 != 0)
        {
            small_break_start_val--;
            reset_start_time(small_break_start_val);
        }
        else if (!study_mode && min_time == big_break_start_val)
        {
            big_break_start_val--;
            reset_start_time(big_break_start_val);
        }
    }
}


// Configure the FPGA interval timer
void set_itimer(void)
{
    volatile int *TIMER_ptr = (int *)TIMER_BASE;
    // set the interval timer period
    int load_val = clock_rate;
    *(TIMER_ptr + 0x2) = (load_val & 0xFFFF);
    *(TIMER_ptr + 0x3) = (load_val >> 16) & 0xFFFF;
    *(TIMER_ptr + 0x0) = 0;
    // turn on CONT & ITO bits (do not start)
    *(TIMER_ptr + 0x1) = 0x3; // STOP = 1, START = 1, CONT = 1, ITO = 1
}

// Configure the 8Mhz, 125us timer for audio in the second interval timer
void set_itimer_audio(void)
{
    volatile int *TIMER_AUDIO_ptr = (int *)TIMER_2_BASE;
    // set the interval timer period
    int load_val = 6250; // for 8Mhz, 125us
    *(TIMER_AUDIO_ptr + 0x2) = (load_val & 0xFFFF);
    *(TIMER_AUDIO_ptr + 0x3) = (load_val >> 16) & 0xFFFF;
    *(TIMER_AUDIO_ptr + 0x0) = 0;
    // turn on CONT & ITO bits (do not start)
    *(TIMER_AUDIO_ptr + 0x1) = 0xF; // STOP = 1, START = 1, CONT = 1, ITO = 1
}

// Configure the KEY port
void set_KEY(void)
{
    volatile int *KEY_ptr = (int *)KEY_BASE;
    *(KEY_ptr + 3) = 0xF; // clear EdgeCapture register
    *(KEY_ptr + 2) = 0xF; // enable interrupts for keys 0-3
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


void draw_hourglass_frame()
{
    draw_line(90, 50, 230, 50, white);   // Top horizontal line
    draw_line(90, 50, 157, 120, white);  // Left diagonal down
    draw_line(230, 50, 163, 120, white); // Right diagonal down

    // thicker
    draw_line(89, 50, 231, 50, white);   // Top horizontal line
    draw_line(89, 50, 156, 120, white);  // Left diagonal down
    draw_line(231, 50, 164, 120, white); // Right diagonal down

    // draw_line(157, 120, 163, 120, 0x8494); // Slight gap in middle

    draw_line(157, 120, 90, 190, white);  // Left diagonal up
    draw_line(163, 120, 230, 190, white); // Right diagonal up
    draw_line(90, 190, 230, 190, white);  // Bottom horizontal line

    //thicker
    draw_line(156, 120, 89, 190, white);  // Left diagonal up
    draw_line(164, 120, 231, 190, white); // Right diagonal up
    draw_line(89, 190, 231, 190, white);  // Bottom horizontal line
}

// void draw_hourglass_frame_big()
// {
//     draw_line(30, 20, 290, 20, 0x8494);   // Top horizontal line
//     draw_line(30, 20, 155, 120, 0x8494);  // Left diagonal down
//     draw_line(290, 20, 165, 120, 0x8494); // Right diagonal down

//     draw_line(155, 120, 165, 120, 0x8494); // Slight gap in middle

//     draw_line(155, 120, 30, 220, 0x8494);  // Left diagonal up
//     draw_line(165, 120, 290, 220, 0x8494); // Right diagonal up
//     draw_line(30, 220, 290, 220, 0x8494);  // Bottom horizontal line
// }


void toggle_display() {
    if (display_mode == 1) {
        display_mode = 2;
    } else if (display_mode == 2) {
        display_mode = 1;
    }
}

void get_hourglass_bounds(int y, int *x_left, int *x_right)
{
    if (y < 120)
    { // Upper part of the hourglass
        *x_left = 90 + ((157 - 90) * (y - 50)) / (120 - 50);
        *x_right = 230 + ((163 - 230) * (y - 50)) / (120 - 50);
    }
    else
    { // Lower part of the hourglass
        *x_left = 157 + ((90 - 157) * (y - 120)) / (190 - 120);
        *x_right = 163 + ((230 - 163) * (y - 120)) / (190 - 120);
    }
}


void setup_hourglass()
{
    for (int y = 60; y < 120; y++)
    {
        int x_left, x_right;
        get_hourglass_bounds(y, &x_left, &x_right);
        draw_line(x_left, y, x_right, y, 0xF691); // Yellow sand
    }
    draw_hourglass_frame();
}


void draw_hourglass_top(int top)
{
    for (int y = top; y < 120; y++)
    {
        int x_left, x_right;
        get_hourglass_bounds(y, &x_left, &x_right);
        draw_line(x_left, y, x_right, y, 0xF691); // Yellow sand
    }
    draw_hourglass_frame();
}


void draw_hourglass_bottom(int apex_y)
{
    int base_y = 190; // Base of triangle is at the bottom of hourglass

    int center_x = 160;
    int max_width = 140;
    int max_triangle_height = 60;
    // Draw triangle at the bottom
    if (hourglass_draw_index > 0)
    {
        // Calculate the apex y-position based on current triangle height
        int apex_y = base_y - hourglass_draw_index;

        // Draw each line of the triangle, narrowing as we go up from the base
        for (int y = base_y; y >= apex_y; y--)
        {
            // Calculate width of this line based on y position
            float progress = (float)(base_y - y) / hourglass_draw_index;

            // Width narrows from current triangle width to a point at the apex
            int triangle_width = (max_width * hourglass_draw_index) / max_triangle_height;
            int line_width = (int)(triangle_width * (1.0 - progress));

            // Center the line in the bottom of the hourglass
            int line_left = center_x - line_width / 2;
            int line_right = center_x + line_width / 2;

            // Draw the line for this row of the triangle
            draw_line(line_left, y, line_right, y, 0xF691); // Yellow sand
        }
    }
}


void draw_hourglass_drip()
{
    draw_line(160, 120 + hourglass_drip_start, 160, 120 + hourglass_drip_end, 0xF691);
    draw_line(159, 120 + hourglass_drip_start, 159, 120 + hourglass_drip_end, 0xF691);
    draw_line(161, 120 + hourglass_drip_start, 161, 120 + hourglass_drip_end, 0xF691);
}


void reset_start_time(int start_time)
{
    min_time = start_time;
    hourglass_sec_to_wait = start_time;
    hourglass_draw_index = 0;
    hourglass_drip_start = 0;
    hourglass_drip_end = 0;
    if (start_time==pom_start_val) {
        colour = red;
    } else if (start_time==small_break_start_val) {
        colour = teal;
    } else {
        colour = navy;
    }
    // clear_screen(colour);
    // wait_for_v_sync();
    // pixel_buffer_start = *(PIXEL_BUF_CTRL_ptr + 1); // new back buffer
    // clear_screen(colour);
    // wait_for_v_sync();
    // pixel_buffer_start = *(PIXEL_BUF_CTRL_ptr + 1); // new back buffer
}
