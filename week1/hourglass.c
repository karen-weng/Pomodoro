#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define SWITCHES_BASE 0xFF200040 // Memory-mapped address for switches
#define PIXEL_BUFFER_BASE 0xFF203020 // Memory-mapped address for pixel buffer

volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUFFER_BASE;
int pixel_buffer_start;
bool hourglass_drawn = false;

void plot_pixel(int x, int y, short int color) {
    volatile short int *pixel_address;
    pixel_address = (volatile short int *)(pixel_buffer_start + (y << 10) + (x << 1));
    *pixel_address = color;
}

void draw_line(int x0, int y0, int x1, int y1, short int color) {
    bool is_steep = abs(y1 - y0) > abs(x1 - x0);
    if (is_steep) {
        int temp = x0; x0 = y0; y0 = temp;
        temp = x1; x1 = y1; y1 = temp;
    }
    if (x0 > x1) {
        int temp = x0; x0 = x1; x1 = temp;
        temp = y0; y0 = y1; y1 = temp;
    }
    int deltax = x1 - x0, deltay = abs(y1 - y0), error = -(deltax / 2), y = y0;
    int y_step = (y0 < y1) ? 1 : -1;
    for (int x = x0; x <= x1; x++) {
        if (is_steep) plot_pixel(y, x, color);
        else plot_pixel(x, y, color);
        error += deltay;
        if (error > 0) {
            y += y_step;
            error -= deltax;
        }
    }
}

void draw_hourglass_frame() {
    draw_line(80, 50, 240, 50, 0xF800);   // Top horizontal line
    draw_line(80, 50, 160, 125, 0xF800);  // Left diagonal down
    draw_line(240, 50, 160, 125, 0xF800); // Right diagonal down
    draw_line(80, 200, 160, 125, 0xF800); // Left diagonal up
    draw_line(240, 200, 160, 125, 0xF800); // Right diagonal up
    draw_line(80, 200, 240, 200, 0xF800); // Bottom horizontal line
}

void draw_sand(uint16_t progress) {
    int fill_height = (progress * 95) / 1023; // Scale progress
    for (int y = 125; y < 125 + fill_height; y++) {
        draw_line(160 - (y - 125), y, 160 + (y - 125), y, 0xFFE0); // Yellow sand filling
    }
}

void wait_for_v_sync() {
    volatile int *fbuf = (int *)PIXEL_BUFFER_BASE;
    int status;
    *fbuf = 1;
    do {
        status = *(fbuf + 3);
    } while (status & 0x01);
}

int main() {
    pixel_buffer_start = *pixel_ctrl_ptr;
    volatile uint16_t *switches = (uint16_t *)SWITCHES_BASE;
    
    if (!hourglass_drawn) {
        draw_hourglass_frame();
        hourglass_drawn = true;
    }
    
    while (1) {
        uint16_t progress = *switches & 0x3FF;
        draw_sand(progress);
        wait_for_v_sync();
    }
    return 0;
}
