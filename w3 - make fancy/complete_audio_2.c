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
#include "samples_rooster_alarm.h" // Include the header file with the list definition
#include "samples_guitar_30sec_44100.h"
#include "samples_colourful_flower_30sec_44100.h"
#include "samples_fluffing_duck_30sec_44100.h"
#include "samples_keyboard_click_louder_44100.h"
#include "samples_keyboard_click_2_44100.h"
#include "samples_beep_beep_louder_44100.h"
#include "samples_school_bell_louder_44100.h"
#include "samples_animal_crossing_30sec_44100.h"



// int rooster_index = 0;
// int school_bell_44100_index = 0;
// int beep_beep_44100_index = 0;
// int boo_44100_index = 0;


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

volatile bool paused = false; // true for pomodoro, false for break,

volatile int *LEDR_ptr = (int *)LEDR_BASE;
volatile int *HEX3_HEX0_ptr = (int *)HEX3_HEX0_BASE;
volatile int *TIMER_ptr = (int *)TIMER_BASE;
volatile int *TIMER_AUDIO_ptr = (int *)TIMER_2_BASE;

volatile int *KEY_ptr = (int *)KEY_BASE;
// vga functions
void plot_pixel(int, int, short int); // plots one pixel
void clear_screen(short int);         // clear whole screen
void clear_rectangle(int, int, int, int, short int);
void display_text(int, int, char *);

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
void reset_hourglass_drip();


void toggle_display();
void change_edit_status(int);
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
int drip_wait_time = 100;

volatile int *PIXEL_BUF_CTRL_ptr = (int *)PIXEL_BUF_CTRL_BASE;
int pixel_buffer_start;      // global variable
short int buffer1[240][512]; // 240 rows, 512 (320 + padding) columns
short int buffer2[240][512];
int colour;
int colour2;
short int black = 0x0;
short int white = 0xFFFF;
short int grey = 0x8494;
short int red = 0xB2EB;
short int dark_red = 0xAA48;
short int teal = 0x5B92;
short int dark_teal = 0x4310;
short int navy = 0x53F3;
short int dark_navy = 0x4372;
int edit_mode = 0;
int delay_count = 0;
int min_digits[2];
int sec_digits[2];
int x0, Y0, x1, Y1;
char modes_text[40] = "Pomodoro     Short Break     Long Break\0";
char clear_text[40] = "                                       \0";
volatile char session_count_text[40] = "                  #X                   \0";
volatile char pomodoro_msg[40] = "            Time to focus!             \0";
volatile char break_msg[40] = "           Time for a break!           \0";

int num_w = 30;
int num_l = 40;
int loading1[] = {100, 130, 220, 150};
int loading2[] = {101, 131, 219, 149};
int area_to_erase[] = {100, 74, 220, 150}; // UPDATE THIS IF TWEAKING DISPLAY LOCATION
int num_coords[] = {132,80,200,120};
int dot1[] = {159, 90, 160, 91};
int dot2[] = {159, 100, 160, 101};
int dot3[] = {159, 25, 160, 26};
int dot4[] = {159, 35, 160, 36};

// audio variables
double volume_factor = 0.5;
bool mute = false;
bool keyboard_pressed = false;
int alarm_mode = 1; // 1 rooster, 2 school bell, 3 beep beep
int study_music_mode = 1; // 1 colourful flowers, 2 guitar
int break_music_mode = 1; // 1 fluffing a duck, 2 animal crossing

int *keyboard_samples;
int keyboard_samples_n;

// most of these are imported through headers

// audio functions
void play_audio_samples(int *samples, int samples_n, int *sample_index);
void play_audio_samples_no_loop(int *samples, int samples_n, int *sample_index, bool *play_audio);
void play_audio_samples_overlay(int *background_samples, int background_samples_n, int *background_sample_index, int *overlay_samples, int overlay_samples_n, int *overlay_sample_index, bool *play_audio);
void reset_alarm_index();


// all lists add here

struct audio_t
{
    volatile unsigned int control;
    volatile unsigned char rarc;
    volatile unsigned char ralc;
    volatile unsigned char warc;
    volatile unsigned char walc;
    volatile unsigned int ldata;
    volatile unsigned int rdata;
};

struct audio_t *const AUDIO_ptr = ((struct audio_t *)AUDIO_BASE);


// image variables
int NOTmute_width = 30;
int NOTmute_height = 27;
const short int NOTmute_data[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

int mute_width = 30;
int mute_height = 27;
const short int mute_data[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

int beep_beep_white_width = 40;
int beep_beep_white_height = 26;
const short int beep_beep_white_data[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

int rooster_image_width = 22;
int rooster_image_height = 28;
const short int rooster_image_data[] = {-1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

int school_bell_outline_width = 28;
int school_bell_outline_height = 23;
const short int school_bell_outline_data[] = {0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1};

// image functions
void draw_image(short int* image_data, int image_width, int image_height, int start_x, int start_y, short int draw_colour);


void play_game(int); // NEW!!!
int game_mode = 0;  // NEW!!!
int rand1;
int rand2;
int answer = 0;
int correct_answer = -1;
int points = 0;     //
char points_msg[40] = "Score:   \0";

int main(void)
{
    /* Declare volatile pointers to I/O registers (volatile means that the
     * accesses will always go to the memory (I/O) address */

    keyboard_samples = keyboard_click_2_44100_samples;
    keyboard_samples_n = keyboard_click_2_44100_num_samples;

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
    colour2 = dark_red;
    tot = min_time*60;
    hourglass_sec_to_wait = pom_start_val;
    pomodoro_msg[27] = (char) 1;
    break_msg[29] = (char) 1;
    rand1 = rand()%9+1;
    rand2 = rand()%9+1;
    correct_answer = rand1*rand2;
    
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
    session_count_text[19] = study_session_count+'0';
    while (1) {
        hex_to_dec(min_time, min_digits);
        hex_to_dec(sec_time, sec_digits);
        wait_for_v_sync();
        pixel_buffer_start = *(PIXEL_BUF_CTRL_ptr + 1); // new back buffer

        //*LEDR_ptr = edit_mode;
        // clear_rectangle(num_coords, colour);
        clear_screen(colour);
        if (display_mode == 1) {
            // clear_rectangle(hourglass_erase, colour); // clear hourglass from toggling
            // clear_rectangle(area_to_erase);
            // clear_rectangle(loading1);
            draw_rectangle(loading1, white); // display loading bar;
            draw_rectangle(loading2, white); // display loading bar;
            if (colour==red) {
                tot = pom_start_val * 60;
                clear_rectangle(80, 17, 119, 27, colour2);
                display_text(21, 45, pomodoro_msg);
            } else if (colour==teal) {
                tot = small_break_start_val * 60;
                clear_rectangle(76+57, 17, 115+68, 27, colour2);
                display_text(21, 45, break_msg);
            } else {
                tot = big_break_start_val * 60;
                clear_rectangle(76+57+62, 17, 115+68+61, 27, colour2);
                display_text(21, 45, break_msg);
            }
            display_text(21, 40, session_count_text);
            for (int i = loading2[1]; i < loading2[3]; i++) {
                int num = (loading2[2] - loading2[0]) * (tot - min_time * 60 - sec_time) / tot + loading2[0];
                draw_line(loading2[0], i, num, i, white);
            }
            display_text(21, 5, modes_text);   // character buffer is 80 by 60
            display_text(21, 57, clear_text);   // character buffer is 80 by 60
            display_text(21, 50, clear_text);
            display_text(21, 52, clear_text);
            display_text(65, 55, clear_text); // NEW!!!
            display_num(loading1[0], loading1[1] - num_l * 1.4, white, min_digits[1]);
            display_num(loading1[0] + num_w, loading1[1] - num_l * 1.4, white, min_digits[0]);
            draw_rectangle(dot1, white);
            draw_rectangle(dot2, white);
            display_num(loading1[2] - num_w * 2, loading1[1] - num_l * 1.4, white, sec_digits[1]);
            display_num(loading1[2] - num_w, loading1[1] - num_l * 1.4, white, sec_digits[0]);
            if (edit_mode==1) { // editing minutes 'ten's
                x0 = loading1[0];
                Y0 = loading1[1]-num_l*1.4;
                x1 = loading1[0]+num_w;
                Y1 = loading1[1]-num_l*1.4+num_l;
                delay_count++; 
            } else if (edit_mode==2) { // editing minutes 'one's
                x0 = loading1[0]+num_w;
                Y0 = loading1[1]-num_l*1.4;
                x1 = loading1[0]+num_w+num_w;
                Y1 = loading1[1]-num_l*1.4+num_l;
                delay_count++;
            }
            if (edit_mode!=0 && delay_count>4) {
                clear_rectangle(x0, Y0, x1, Y1, colour2);
            }
            display_num(loading1[0], loading1[1] - num_l * 1.4, white, min_digits[1]);
            display_num(loading1[0] + num_w, loading1[1] - num_l * 1.4, white, min_digits[0]);
            draw_rectangle(dot1, white);
            draw_rectangle(dot2, white);
            display_num(loading1[2] - num_w * 2, loading1[1] - num_l * 1.4, white, sec_digits[1]);
            display_num(loading1[2] - num_w, loading1[1] - num_l * 1.4, white, sec_digits[0]);
        }
        else if (display_mode == 2) {
            if (colour==red) {
                tot = pom_start_val * 60;
                clear_rectangle(80, 225, 119, 235, colour2);
                display_text(21, 52, pomodoro_msg);
            } else if (colour==teal) {
                tot = small_break_start_val * 60;
                clear_rectangle(80+53, 225, 119+64, 235, colour2);
                display_text(21, 52, break_msg);
            } else {
                tot = big_break_start_val * 60;
                clear_rectangle(80+53+62, 225, 119+64+61, 235, colour2);
                display_text(21, 52, break_msg);
            }

            display_text(21, 50, session_count_text);

            // clear_rectangle(hourglass_erase, colour);
            draw_hourglass_top(61 + hourglass_draw_index);
            draw_hourglass_bottom(190 - hourglass_draw_index);
            draw_hourglass_drip();
            draw_hourglass_frame();

            if (edit_mode==1) { // editing minutes 'ten's
                x0 = loading1[0];
                Y0 = loading1[1] - num_l * 3.1;
                x1 = loading1[0]+num_w;
                Y1 = loading1[1] - num_l * 3.1+num_l;
                delay_count++; 
            } else if (edit_mode==2) { // editing minutes 'one's
                x0 = loading1[0]+num_w;
                Y0 = loading1[1] - num_l * 3.1;
                x1 = loading1[0]+num_w+num_w;
                Y1 = loading1[1] - num_l * 3.1+num_l;
                delay_count++;
            }
            if (edit_mode!=0 && delay_count>4) {
                clear_rectangle(x0, Y0, x1, Y1, colour2);
            }
            display_text(21, 5, clear_text);   // character buffer is 80 by 60
            display_text(21, 40, clear_text);
            display_text(21, 45, clear_text);
            display_text(65, 55, clear_text); // NEW!!!
            display_text(21, 57, modes_text);   // character buffer is 80 by 60
            display_num(loading1[0], loading1[1] - num_l * 3.1, white, min_digits[1]);
            display_num(loading1[0] + num_w, loading1[1] - num_l * 3.1, white, min_digits[0]);
            draw_rectangle(dot3, white);
            draw_rectangle(dot4, white);
            display_num(loading1[2] - num_w * 2, loading1[1] - num_l * 3.1, white, sec_digits[1]);
            display_num(loading1[2] - num_w, loading1[1] - num_l * 3.1, white, sec_digits[0]);
        } else if (display_mode==3) {   // NEW!!!
            draw_rectangle(loading1, white); // display loading bar;
            draw_rectangle(loading2, white); // display loading bar;
            if (colour==teal) {
                tot = small_break_start_val * 60;
                clear_rectangle(76+57, 17, 115+68, 27, colour2);
                display_text(21, 45, break_msg);
            } else if (colour==navy) {
                tot = big_break_start_val * 60;
                clear_rectangle(76+57+62, 17, 115+68+61, 27, colour2);
                display_text(21, 45, break_msg);
            }
            display_text(21, 40, session_count_text);
            for (int i = loading2[1]; i < loading2[3]; i++) {
                int num = (loading2[2] - loading2[0]) * (tot - min_time * 60 - sec_time) / tot + loading2[0];
                draw_line(loading2[0], i, num, i, white);
            }
            display_text(21, 5, modes_text);   // character buffer is 80 by 60
            display_text(21, 57, clear_text);   // character buffer is 80 by 60
            display_text(21, 50, clear_text);
            display_text(21, 52, clear_text);
            if (game_mode==1) { // editing minutes 'ten's
                x0 = loading1[0] + num_w * 2.5;
                Y0 = loading1[1]-num_l*1.4;
                x1 = loading1[0] + num_w * 3.3;
                Y1 = loading1[1]-num_l*1.4+num_l;
                delay_count++; 
            } else if (game_mode==2) { // editing minutes 'one's
                x0 = loading1[2] - num_w * 0.7;
                Y0 = loading1[1]-num_l*1.4;
                x1 = loading1[2] + num_w * 0.1;
                Y1 = loading1[1]-num_l*1.4+num_l;
                delay_count++;
            } else if (game_mode==3) {
                delay_count++;
            }
            if (game_mode!=0 && delay_count>4) {
                clear_rectangle(x0, Y0, x1, Y1, colour2);
                display_text(65, 55, points_msg);
            } else {
                display_text(65, 55, clear_text);
            }
            display_num(loading1[0] - num_w * 0.2, loading1[1] - num_l * 1.4, white, rand1);
            display_num(loading1[0] + num_w * 1.2, loading1[1] - num_l * 1.4, white, rand2);
            draw_line(125, 89, 135, 99, white); // multiplication sign
            draw_line(125, 90, 136, 99, white);
            draw_line(135, 89, 125, 99, white);
            draw_line(136, 89, 126, 99, white);

            draw_line(163, 90, 173, 90, white); // equal sign
            draw_line(163, 91, 173, 91, white);
            draw_line(163, 96, 173, 96, white);
            draw_line(163, 97, 173, 97, white);
            display_num(loading1[2] - num_w * 0.8, loading1[1] - num_l * 1.4, white, answer/10);
            display_num(loading1[0] + num_w * 2.4, loading1[1] - num_l * 1.4, white, answer%10);
        }

        // draw image icons
        
        if (mute) {
            draw_image(mute_data, mute_width, mute_height, 10, 10, -1);
        }
        else{
            draw_image(NOTmute_data, NOTmute_width, NOTmute_height, 10, 10, -1);
        }
        
        if (alarm_mode == 1) {
            draw_image(rooster_image_data, rooster_image_width, rooster_image_height, 280, 8, -1);
        }
        else if (alarm_mode == 2) {
            draw_image(school_bell_outline_data, school_bell_outline_width, school_bell_outline_height, 280, 12, -1);
        }
        else if (alarm_mode == 3) {
            draw_image(beep_beep_white_data, beep_beep_white_width, beep_beep_white_height, 270, 10, -1);
        }
    }
}

void wait_for_v_sync()
{
    volatile int *fbuf = (int *)PIXEL_BUF_CTRL_BASE;
    *fbuf = 1;
    int status = *(fbuf + 3);
    while ((status & 0x01) != 0)
    {
        status = *(fbuf + 3);
    }
}

void plot_pixel(int x, int y, short int line_color)
{
    int offset = (y << 10) + (x << 1);
    *(volatile short int *)(pixel_buffer_start + offset) = line_color;
}

void draw_line(int x0, int y0, int x1, int y1, short int colour)
{
    if (x0 == x1)
    {
        for (int r = y0; r <= y1; r++)
            plot_pixel(x0, r, colour);
    }
    else if (y0 == y1)
    {
        for (int c = x0; c <= x1; c++)
            plot_pixel(c, y0, colour);
    }
    else
    {
        bool is_steep = abs(y1 - y0) > abs(x1 - x0);
        if (is_steep)
        {
            int temp = x0;
            x0 = y0;
            y0 = temp;
            temp = x1;
            x1 = y1;
            y1 = temp;
        }
        if (x0 > x1)
        {
            int temp = x0;
            x0 = x1;
            x1 = temp;
            temp = y0;
            y0 = y1;
            y1 = temp;
        }
        int deltax = x1 - x0, deltay = abs(y1 - y0), error = -(deltax / 2), y = y0;
        int y_step = (y0 < y1) ? 1 : -1;
        for (int x = x0; x <= x1; x++)
        {
            if (is_steep)
                plot_pixel(y, x, colour);
            else
                plot_pixel(x, y, colour);
            error += deltay;
            if (error > 0)
            {
                y += y_step;
                error -= deltax;
            }
        }
    }
}

void draw_rectangle(int coords[], short int colour)
{                                                                  // empty rectangle (not filled)
    draw_line(coords[0], coords[1], coords[2], coords[1], colour); // display loading bar
    draw_line(coords[0], coords[3], coords[2], coords[3], colour);
    draw_line(coords[0], coords[1], coords[0], coords[3], colour);
    draw_line(coords[2], coords[1], coords[2], coords[3], colour);
}

void clear_screen(short int c) {
    clear_rectangle(0, 0, 319, 239, c); // ERROR
}

void clear_rectangle(int x0, int y0, int x1, int y1, short int c) {
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++)
            plot_pixel(x, y, c);
}

void display_num(int x, int y, short int line_color, int num)
{
    if (num != 1 && num != 4)
    { // seg 0: 0 2 3 5 6 7 8 9
        for (int r = y + 2; r < y + 4; r++)
            for (int c = x + 8; c < x + num_w - 8; c++)
                plot_pixel(c, r, line_color);
    }
    if (num != 5 && num != 6)
    { // seg 1: 0 1 2 3 4 7 8 9
        for (int r = y + 4; r < y + 19; r++)
            for (int c = x + num_w - 8; c < x + num_w - 6; c++)
                plot_pixel(c, r, line_color);
    }
    if (num != 2)
    { // seg 2: 0 1 3 4 5 6 7 8 9
        for (int r = y + 21; r < y + num_l - 4; r++)
            for (int c = x + num_w - 8; c < x + num_w - 6; c++)
                plot_pixel(c, r, line_color);
    }
    if (num != 1 && num != 4 && num != 7)
    { // seg 3: 0 2 3 5 6 8 9
        for (int r = y + 36; r < y + num_l - 2; r++)
            for (int c = x + 8; c < x + num_w - 8; c++)
                plot_pixel(c, r, line_color);
    }
    if (num == 0 || num == 2 || num == 6 || num == 8)
    { // seg 4: 0 2 6 8
        for (int r = y + 21; r < y + num_l - 4; r++)
            for (int c = x + 6; c < x + 8; c++)
                plot_pixel(c, r, line_color);
    }
    if (num != 1 && num != 2 && num != 3 && num != 7)
    { // seg 5: 0 4 5 6 8 9
        for (int r = y + 4; r < y + 19; r++)
            for (int c = x + 6; c < x + 8; c++)
                plot_pixel(c, r, line_color);
    }
    if (num != 0 && num != 1 && num != 7)
    { // seg 6: 2 3 4 5 6 8 9
        for (int r = y + 19; r < y + 21; r++)
            for (int c = x + 8; c < x + num_w - 8; c++)
                plot_pixel(c, r, line_color);
    }
}

void hex_to_dec(int hex_value, int digits[])
{
    if (hex_value > 99)
    {
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


void play_audio_samples_no_loop(int *samples, int samples_n, int *sample_index, bool *play_audio)
{
    if (*sample_index < samples_n)
    {
        if (AUDIO_ptr->warc > 0)
        {
            int scaled_sample = (int)(samples[*sample_index] * volume_factor);

            AUDIO_ptr->ldata = scaled_sample;
            AUDIO_ptr->rdata = scaled_sample;

            (*sample_index)++;
            if (*sample_index >= samples_n)
            {
                *play_audio = false; // Stop playing audio
                *sample_index = 0;
            }
        }
    }
}

void play_audio_samples_overlay(int *background_samples, int background_samples_n, int *background_sample_index, int *overlay_samples, int overlay_samples_n, int *overlay_sample_index, bool *play_audio)
{
    if (*overlay_sample_index < overlay_samples_n)
    {
        if (AUDIO_ptr->warc > 0)
        {
            int scaled_sample = (int)(background_samples[*background_sample_index] * 0.3 + overlay_samples[*overlay_sample_index]);

            AUDIO_ptr->ldata = scaled_sample;
            AUDIO_ptr->rdata = scaled_sample;

            (*overlay_sample_index)++;
            if (*overlay_sample_index >= overlay_samples_n)
            {
                *play_audio = false; // Stop playing audio
                *overlay_sample_index = 0;
            }
        }
    }
}

// audio ISR, techcially timer
void audio_ISR_timer2(void)
{
    *TIMER_AUDIO_ptr = 0; // reset interrupt
    // already set to continue timer
    // TODO add mute variable
    if (mute) {
        if (keyboard_pressed) {
            play_audio_samples_no_loop(keyboard_samples, keyboard_samples_n, &keyboard_click_louder_44100_index, &keyboard_pressed);
        }
    }
    else
    {
        if (keyboard_pressed) {
            if (key_mode == 2)
            {
                if (study_mode == true) { // studying               
                    if (study_music_mode == 1) {
                        play_audio_samples_overlay(colourful_flower_30sec_44100_samples, colourful_flower_30sec_44100_num_samples, &colourful_flower_30sec_44100_index, keyboard_samples, keyboard_samples_n, &keyboard_click_louder_44100_index, &keyboard_pressed);
                    }
                    else if (study_music_mode == 2) {
                        play_audio_samples_overlay(guitar_30sec_44100_samples, guitar_30sec_44100_num_samples, &guitar_30sec_44100_index, keyboard_samples, keyboard_samples_n, &keyboard_click_louder_44100_index, &keyboard_pressed);
                    }
                } // if counting

                else if (!paused) {
                    // fluffing duck
                    // break mode{
                    if (break_music_mode == 1) {
                        play_audio_samples_overlay(fluffing_duck_30sec_44100_samples, fluffing_duck_30sec_44100_num_samples, &fluffing_duck_30sec_44100_index, keyboard_samples, keyboard_samples_n, &keyboard_click_louder_44100_index, &keyboard_pressed);    
                    }
                    else if (break_music_mode == 2) {
                        play_audio_samples_overlay(animal_crossing_30sec_44100_samples, animal_crossing_30sec_44100_num_samples, &animal_crossing_30sec_44100_index, keyboard_samples, keyboard_samples_n, &keyboard_click_louder_44100_index, &keyboard_pressed);    
                    }
                }
            }
    
            else if (key_mode == 3) { // alarm
                // rooster
                if (alarm_mode == 1) {
                    play_audio_samples_overlay(rooster_samples, rooster_num_samples, &rooster_index, keyboard_samples, keyboard_samples_n, &keyboard_click_louder_44100_index, &keyboard_pressed);
                }
                else if (alarm_mode == 2) {
                    // school bell
                    // TODO school bell louder, beep beep louder
                    play_audio_samples_overlay(school_bell_louder_44100_samples, school_bell_louder_44100_num_samples, &school_bell_louder_44100_index, keyboard_samples, keyboard_samples_n, &keyboard_click_louder_44100_index, &keyboard_pressed);
                }
                else if (alarm_mode == 3) {
                    // beep beep
                    play_audio_samples_overlay(beep_beep_louder_44100_samples, beep_beep_louder_44100_num_samples, &beep_beep_louder_44100_index, keyboard_samples, keyboard_samples_n, &keyboard_click_louder_44100_index, &keyboard_pressed);
                }
            }
            else {
                play_audio_samples_no_loop(keyboard_samples, keyboard_samples_n, &keyboard_click_louder_44100_index, &keyboard_pressed);
            }
        }
        // no keyboard sound effect
        else {
            if (key_mode == 2) {
                if (study_mode == true) {
                    // studying
                    if (study_music_mode == 1) {
                        play_audio_samples(colourful_flower_30sec_44100_samples, colourful_flower_30sec_44100_num_samples, &colourful_flower_30sec_44100_index);
                    }
                    else if (study_music_mode == 2) {
                        play_audio_samples(guitar_30sec_44100_samples, guitar_30sec_44100_num_samples, &guitar_30sec_44100_index);
                    }
                }

                else if (!paused) {
                    // break mode
                    // fluffing duck

                    if (break_music_mode == 1) {
                        play_audio_samples(fluffing_duck_30sec_44100_samples, fluffing_duck_30sec_44100_num_samples, &fluffing_duck_30sec_44100_index);
                    }
                    else if (break_music_mode == 2) {
                        play_audio_samples(animal_crossing_30sec_44100_samples, animal_crossing_30sec_44100_num_samples, &animal_crossing_30sec_44100_index);
                    }
                }
            }
            else if (key_mode == 3) { // alarm
                // rooster
                if (alarm_mode == 1) {
                    play_audio_samples(rooster_samples, rooster_num_samples, &rooster_index);
                }
                else if (alarm_mode == 2) {
                    // school bell
                    // TODO school bell louder, beep beep louder
                    play_audio_samples(school_bell_louder_44100_samples, school_bell_louder_44100_num_samples, &school_bell_louder_44100_index);
                }
                else if (alarm_mode == 3) {
                    // beep beep
                    play_audio_samples(beep_beep_louder_44100_samples, beep_beep_louder_44100_num_samples, &beep_beep_louder_44100_index);
                }
            }
        }
    }

    // ^ THAT IS IMPORTANT IT IS AUDIO ITS JUST COMPONENTED OUT CAUSE ITS SLOW

    if (key_mode == 2)
    { // if counting
        if (drip_wait_counter < drip_wait_time)
        {
            drip_wait_counter++;
        }
        else
        {
            drip_wait_counter = 0;
            if ((hourglass_drip_end < 70) && (hourglass_draw_index <= 1))
            {
                hourglass_drip_end++;
            }
            else if ((hourglass_draw_index >= 59) && (hourglass_drip_start < 70))
            {
                hourglass_drip_start++;
            }
        }
    }
}

// KEY port interrupt service routine
void KEY_ISR(void)
{
    int pressed_key;
    pressed_key = *(KEY_ptr + 3); // read EdgeCapture & get key value
    *(KEY_ptr + 3) = pressed_key; // clear EdgeCapture register
    if (pressed_key == 1)
    { // user presses start/pause/stop
        if (key_mode == 1)
        {                             // start
            *(TIMER_ptr + 0x1) = 0x7; // 0b0111 (start, cont, ito)
            key_mode = 2;
        }
        else if (key_mode == 2)
        {                             // pause
            *(TIMER_ptr + 0x1) = 0xB; // 0b1011 (stop, cont, ito)
            key_mode = 1;
        }
        else if (key_mode == 3)
        { // update next countdown start value
            game_mode = 0;
            study_mode = !study_mode;
            if (study_mode)
            {
                min_time = pom_start_val;
                study_session_count++;
                if (study_session_count>8) {
                    study_session_count = 1;
                }
                session_count_text[19] = study_session_count+'0';
            }
            else if (!study_mode && study_session_count % 4 != 0)
            {
                min_time = small_break_start_val;
            }
            else if (!study_mode)
            {
                min_time = big_break_start_val;
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
    else if (pressed_key == 2)
    { // skip
        *(TIMER_ptr + 0x1) = 0xB;
        key_mode = 1; // auto-set to start
        study_mode = !study_mode;
        sec_time = 0;
        if (study_mode)
        {
            min_time = pom_start_val;
            study_session_count++;
            if (study_session_count>8) {
                study_session_count = 1;
            }
            session_count_text[19] = study_session_count+'0';
        }
        else if (!study_mode && study_session_count % 4 != 0)
        {
            min_time = small_break_start_val;
        }
        else if (!study_mode)
        {
            min_time = big_break_start_val;
        }
        else
        {
            printf("Unexpected study mode %d.", study_mode);
        }
        // for increasing & decreasing start value, check mode & if haven't ever pressed start
    }
    else if (pressed_key == 4 && key_mode == 1)
    { // increase start value
        if (study_mode && min_time == pom_start_val)
        {
            pom_start_val++;
            min_time = pom_start_val;
        }
        else if (!study_mode && min_time == small_break_start_val && study_session_count % 4 != 0)
        {
            small_break_start_val++;
            min_time = small_break_start_val;
        }
        else if (!study_mode && min_time == big_break_start_val)
        {
            big_break_start_val++;
            min_time = big_break_start_val;
        } // else user already started timer once, needs to skip/finish current session
    }
    else if (pressed_key == 8 && key_mode == 1)
    { // decrease start value
        if (study_mode && min_time == pom_start_val)
        {
            pom_start_val--;
            min_time = pom_start_val;
        }
        else if (!study_mode && min_time == small_break_start_val && study_session_count % 4 != 0)
        {
            small_break_start_val--;
            min_time = small_break_start_val;
        }
        else if (!study_mode && min_time == big_break_start_val && study_session_count%4==0)
        {
            big_break_start_val--;
            min_time = big_break_start_val;
        }
    }
    else
    {
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
        keyboard_pressed = true; // only play sound on release 
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
                change_edit_status(0);
                play_game(0);
                break; // 0
            case 0x16:
                change_edit_status(1);
                play_game(1);
                // led_display_val = 1;
                break; // 1
            case 0x1E:
                change_edit_status(2);
                play_game(2);
                // led_display_val = 2;
                break; // 2
            case 0x26:
                change_edit_status(3);
                play_game(3);
                // led_display_val = 3;
                break; // 3
            case 0x25:
                change_edit_status(4); 
                play_game(4);
                // led_display_val = 4;
                break; // 4
            case 0x2E:
                change_edit_status(5);
                play_game(5);
                // led_display_val = 5;
                break; // 5
            case 0x36:
                change_edit_status(6);
                play_game(6);
                // led_display_val = 6;
                break; // 6
            case 0x3D:
                change_edit_status(7);
                play_game(7);
                // led_display_val = 7;
                break; // 7
            case 0x3E:
                change_edit_status(8);
                play_game(8);
                // led_display_val = 8;
                break; // 8
            case 0x46:
                change_edit_status(9);
                play_game(9);
                // led_display_val = 9;
                break; // 9

            // letters
            case 0x24:  // E
                if (edit_mode==0) {
                    change_edit_status(-1);
                } else {
                    change_edit_status(-2);
                }
                break;
                case 0x2D: // right now deosnt care if it is currently alarm or not
                // led_display_val = 256;
                // recording = false;
                *(TIMER_ptr + 0x1) = 0xB; // 0b1011 (stop, cont, ito)
                key_mode = 1;
                sec_time = 0;
                reset_hourglass_drip();
                if (colour==red) {
                    min_time = pom_start_val;
                } else if (colour==teal) {
                    min_time = small_break_start_val;
                } else {
                    min_time = big_break_start_val;
                }
                break;  // R -- custom reset
            case 0x2C:   // T -- global reset
                *(TIMER_ptr + 0x1) = 0xB; // 0b1011 (stop, cont, ito)
                key_mode = 1;
                study_mode = true;
                pom_start_val = 25;
                colour = red;
                display_mode = 1;
                edit_mode = 0;
                game_mode = 0;
                small_break_start_val = 5;
                big_break_start_val = 15;
                min_time = pom_start_val;
                sec_time = 0;
                study_session_count = 1;
                session_count_text[19] = study_session_count+'0';
                reset_start_time(min_time);
                //reset_alarm_index();
                break;  

                // alarm modes sound effects
            case 0x43: 
                alarm_mode = 1;
                reset_alarm_index();
                break; // I
            case 0x44: 
                alarm_mode = 2;
                reset_alarm_index();
                break; // O  
            case 0x4D:
                alarm_mode = 3;
                reset_alarm_index();
                break; // P

                // study music modes

            case 0x42: 
                study_music_mode = 1;
                break; // K  
            case 0x4B:
                study_music_mode = 2;
                break; // L

                // break music
            case 0x32: 
                break_music_mode = 1;
                break; // N
            case 0x3A:
                break_music_mode = 2;
                break; // M

                // keyboard click sound effects
            case 0x41: // 
                // keyboard click variant 1
                keyboard_samples = keyboard_click_louder_44100_samples;
                keyboard_samples_n = keyboard_click_louder_44100_num_samples;
                
                break; // <
            case 0x49: // 
                // keyboard click variant 2
                keyboard_samples = keyboard_click_2_44100_samples;
                keyboard_samples_n = keyboard_click_2_44100_num_samples;
                break; // >


            // function keys
            // 0x05, 0x06, 0x04
            case 0x05:
                // led_display_val = 256;
                mute = !mute;
                break; // F1 // mute
            case 0x06:
                // led_display_val = 256;
                if (volume_factor > 0.1)
                {
                    volume_factor -= 0.1;
                }
                break; // F2 // decrease volume
            case 0x04:
                // led_display_val = 256;
                if (volume_factor < 0.9)
                {
                    volume_factor += 0.1;
                }
                break; // F3 // increase volume

            // other // enter, tab, space, backspace
            // 0x5A, 0x0D, 0x29, 0x66
            case 0x5A:
                // led_display_val = 512;
                change_edit_status(-2);     // exit edit mode
                pressed_enter();
                break; // enter
            case 0x0D:
                // led_display_val = 512;
                change_edit_status(-2);     // exit edit mode
                pressed_tab();
                break; // tab
            case 0x29:
                // led_display_val = 256;
                change_edit_status(-2);     // exit edit mode
                pressed_enter();
                break; // space
            case 0x66:
                // led_display_val = 256;
                change_edit_status(10);
                break; // backspace
            }
        }
    }
    else // holding key
    {
        if (byte3 == 0x2D)
        {
            // led_display_val = 256;
            // recording = true;
        }
    }
}

void display_text(int x, int y, char * text_ptr) {
    int offset;
    volatile char * character_buffer =
    (char *)FPGA_CHAR_BASE; // video character buffer
    /* assume that the text string fits on one line */
    offset = (y << 7) + x;
    while (*(text_ptr)) {
        *(character_buffer + offset) =
        *(text_ptr); // write to the character buffer
        ++text_ptr;
        ++offset;
    }
}

void change_edit_status(int num) {
    // if edit mode is zero, not editing, 1 for tens, 2 for ones
    int start;
    if (colour==red) {
        start = pom_start_val;
    } else if (colour==teal) {
        start = small_break_start_val;
    } else {
        start = big_break_start_val;
    }
    if (key_mode==1 && min_time==start) {
        if (num==-2) {      // stop editing
            edit_mode = 0;
            if (min_time==0) {  // if incorrect input of no minutes, set to default
                if (colour==red) {
                    min_time = 25;
                    pom_start_val = min_time;
                } else if (colour==teal) {
                    min_time = 5;
                    small_break_start_val = min_time;
                } else {
                    min_time = 15;
                    big_break_start_val = min_time;
                }
            }
        } else if (num==-1) {   // start editing
            edit_mode = 1;
            delay_count = 0;
        } else {
            if (edit_mode==1) {
                edit_mode = 2;
                if (num==10) {
                    edit_mode = 1;
                    num = 0;
                } else {
                    delay_count = 0;
                }
                min_time = num*10+min_time%10;
            } else if (edit_mode==2) {
                edit_mode = 1;
                delay_count = 0;
                if (num==10) {
                    num = 0;
                }
                min_time = (min_time - min_time%10) + num;
            }
            if (colour==red) {
                pom_start_val = min_time;
            } else if (colour==teal) {
                small_break_start_val = min_time;
            } else {
                big_break_start_val = min_time;
            }
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
        paused = false;
    }
    else if (key_mode == 2)
    {                             // pause
        *(TIMER_ptr + 0x1) = 0xB; // 0b1011 (stop, cont, ito)
        key_mode = 1;
        paused = true;
    }
    else if (key_mode == 3)
    { // update next countdown start value
        study_mode = !study_mode;
        if (study_mode)
        {
            reset_start_time(pom_start_val);

            study_session_count++;
            if (study_session_count>8) {
                study_session_count = 1;
            }
            session_count_text[19] = study_session_count+'0';
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
        paused = false;

        // reset all audio indexes
        // TODO make into a function
        reset_alarm_index();
    }
    else
    {
        printf("Unexpected key mode %d.", key_mode);
    }
}

void pressed_tab(void)
{ // skip
    *(TIMER_ptr + 0x1) = 0xB;
    key_mode = 1; // auto-set to start // not counting
    study_mode = !study_mode;    
    sec_time = 0;
    if (study_mode)
    {
        reset_start_time(pom_start_val);

        study_session_count++;
        if (study_session_count>8) {
            study_session_count = 1;
        }
        session_count_text[19] = study_session_count+'0';
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

void toggle_display()
{
    if (colour==red) {
        if (display_mode == 1) {
            display_mode = 2;
        } else if (display_mode == 2 || display_mode == 3) {
            display_mode = 1;
        }
    } else {    // does not consider if left or right arrow key
        if (display_mode < 3 ) {
            display_mode++;
        } else if (display_mode == 3) {
            display_mode = 1;
        }
        if (display_mode==3) {
            game_mode = 3;
        }
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
    reset_hourglass_drip();
    if (start_time==pom_start_val) {
        colour = red;
        colour2 = dark_red;
    } else if (start_time==small_break_start_val && study_session_count%4!=0) {
        colour = teal;
        colour2 = dark_teal;
    } else {
        colour = navy;
        colour2 = dark_navy;
    }
}

void reset_hourglass_drip() {
    hourglass_draw_index = 0;
    hourglass_drip_start = 0;
    hourglass_drip_end = 0;
}

void reset_alarm_index()
{
    rooster_index = 0;
    school_bell_louder_44100_index = 0;
    beep_beep_louder_44100_index = 0;
}

void draw_image(short int* image_data, int image_width, int image_height, int start_x, int start_y, short int draw_colour)
{
    for (int y = 0; y < image_height; y++)
    {
        for (int x = 0; x < image_width; x++)
        {
            int index = y * image_width + x; // 1D index for 2D image
            short int image_colour = image_data[index]; // Read image_colour from array

            if (image_colour != -1) // Skip white pixels (transparent effect)
            {
                if (draw_colour != 0) { // set your own colour if not black
                    plot_pixel(start_x + x, start_y + y, draw_colour); // Draw pixel
                }
                else {
                    plot_pixel(start_x + x, start_y + y, image_colour); // Draw pixel
                }
            }
        }
    }
}

void play_game(int num) {   // NEW!!!
    // 1 for tens, 2 for ones, 
    if (key_mode==2) {
        if (game_mode==1) {
            answer = num;
            game_mode = 2;
        } else if (game_mode==2) {
            answer = answer*10+num;
            game_mode = 3;
        }
        if (game_mode==3 || answer==correct_answer || answer>correct_answer) {
            if (answer==correct_answer) {
                points++;
            }
            rand1 = rand()%9+1;
            rand2 = rand()%9+1;
            correct_answer = rand1*rand2;
            answer = 0;
            game_mode = 1;
            points_msg[8] = (points/10)+'0';
            points_msg[9] = (points%10)+'0';
        }
    }
}