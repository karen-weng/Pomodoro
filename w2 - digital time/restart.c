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

void plot_pixel(int, int, short int); // plots one pixel
void clear_screen(); // clears whole screen
void display_num(int, int, short int, int);
void wait_for_v_sync();
volatile int* PIXEL_BUF_CTRL_ptr = (int*) PIXEL_BUF_CTRL_BASE;
int pixel_buffer_start; // global variable

int main(void) {
    /* Read location of the pixel buffer from the pixel buffer controller */
    pixel_buffer_start = *PIXEL_BUF_CTRL_ptr;

    // 319, 239
    clear_screen();
    //draw_line(0, y_coor, 319, y_coor, 0x07E0);   // this line is green
    while (1) {
        
        
        *PIXEL_BUF_CTRL_ptr = 1;
        wait_for_v_sync();
        //clear_screen();


    }

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
        for (int r=x+2; r<x+4; r++) {
            for (int c=y+4; c<y+18; c++) {
                plot_pixel(c, r, line_color);
            }
        }
    }
    if (num!=5 && num!=6) { // seg 1: 0 1 2 3 4 7 8 9
        for (int r=x+4; r<x+19; r++) {
            for (int c=y+18; c<y+20; c++) {
                plot_pixel(c, r, line_color);
            }
        }
    }
    if (num!=2) { // seg 2: 0 1 3 4 5 6 7 8 9
        for (int r=x+21; r<x+36; r++) {
            for (int c=y+18; c<y+20; c++) {
                plot_pixel(c, r, line_color);
            }
        }
    }
    if (num!=1 && num!=4 && num!=7) { // seg 3: 0 2 3 5 6 8 9
        for (int r=x+36; r<x+38; r++) {
            for (int c=y+4; c<y+18; c++) {
                plot_pixel(c, r, line_color);
            }
        }
    }
    if (num==0 || num==2 || num==6 || num==8) { // seg 4: 0 2 6 8
        for (int r=x+21; r<x+36; r++) {
            for (int c=y+2; c<y+4; c++) {
                plot_pixel(c, r, line_color);
            }
        }
    }
    if (num!=1 && num!=2 && num!=3 && num!=7) { // seg 5: 0 4 5 6 8 9
        for (int r=x+4; r<x+19; r++) {
            for (int c=y+2; c<y+4; c++) {
                plot_pixel(c, r, line_color);
            }
        }
    }
    if (num!=0 && num!=1 && num!=7) { // seg 6: 2 3 4 5 6 8 9
        for (int r=x+19; r<x+21; r++) {
            for (int c=y+4; c<y+18; c++) {
                plot_pixel(c, r, line_color);
            }
        }
    }
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