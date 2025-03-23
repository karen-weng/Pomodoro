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
#include <math.h>

#define clock_rate 100000000
#define quarter_clock clock_rate / 4

static void handler(void) __attribute__ ((interrupt ("machine")));
void set_itimer(void);
void set_KEY(void);
void itimer_ISR(void);
void KEY_ISR(void);

void plot_pixel(int x, int y, short int line_color); // plots one pixel
void clear_screen(); // clears whole screen
void swap(int *num1, int *num2);
void draw_line(int x0,int  y0,int x1,int y1, short int colour);
void wait_for_v_sync();
void draw_box(int x, int y, short int colour) ;


int pixel_buffer_start; // global variable
short int Buffer1[240][512]; // 240 rows, 512 (320 + padding) columns
short int Buffer2[240][512];

volatile int pom_start_val = 25;
volatile int small_break_start_val = 5;
volatile int big_break_start_val = 15;
volatile int sec_time = 0;
volatile int min_time = 0;
volatile int key_mode = 1;  // 1 for start, 2 for pause, 3 for stop (ringing) --> multiply by 2 for break vals
volatile bool study_mode = true; // 0 for pomodoro, 1 for break, 2 for long break
volatile int study_session_count = 1;


int main(void) {
    /* Declare volatile pointers to I/O registers (volatile means that the
     * accesses will always go to the memory (I/O) address */
    volatile int *LEDR_ptr = (int *) LEDR_BASE;
    volatile int *HEX3_HEX0_ptr = (int *) HEX3_HEX0_BASE;
    volatile int * CHAR_BUF_CTRL_ptr = (int *)CHAR_BUF_CTRL_BASE;
    

    sec_time = pom_start_val;

    while (1) {
        *LEDR_ptr = sec_time;
    }
}