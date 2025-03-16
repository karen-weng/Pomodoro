#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#define SWITCHES_BASE 0xFF200040     // Memory-mapped address for switches
#define PIXEL_BUFFER_BASE 0xFF203020 // Memory-mapped address for pixel buffer

// volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUFFER_BASE;
int pixel_buffer_start;
bool hourglass_drawn = false;

void plot_pixel(int x, int y, short int color)
{
    volatile short int *pixel_address;
    pixel_address = (volatile short int *)(pixel_buffer_start + (y << 10) + (x << 1));
    *pixel_address = color;
}

void draw_line(int x0, int y0, int x1, int y1, short int color)
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
            plot_pixel(y, x, color);
        else
            plot_pixel(x, y, color);
        error += deltay;
        if (error > 0)
        {
            y += y_step;
            error -= deltax;
        }
    }
}

void draw_hourglass_frame()
{
    draw_line(90, 50, 230, 50, 0xF800);   // Top horizontal line
    draw_line(90, 50, 155, 120, 0xF800);  // Left diagonal down
    draw_line(230, 50, 165, 120, 0xF800); // Right diagonal down

    draw_line(155, 120, 165, 120, 0xF800); // Slight gap in middle

    draw_line(155, 120, 90, 190, 0xF800);  // Left diagonal up
    draw_line(165, 120, 230, 190, 0xF800); // Right diagonal up
    draw_line(90, 190, 230, 190, 0xF800);  // Bottom horizontal line
}

void wait_for_v_sync()
{
    volatile int *fbuf = (int *)0xFF203020;
    int status;
    *fbuf = 1;
    status = *(fbuf + 3);
    while ((status & 0x01) != 0)
    {
        status = *(fbuf + 3);
    }
}

void clear_screen()
{
    int y, x;
    for (x = 0; x < 320; x++)
        for (y = 0; y < 240; y++)
            plot_pixel(x, y, 0);
}

void fill_trapezoid(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, short int color)
{
    // top left, top right, bottom right, bottom left
    for (int y = y0; y <= y3; y++)
    {
        int start_x = x0 + ((x3 - x0) * (y - y0)) / (y3 - y0);
        int end_x = x1 + ((x2 - x1) * (y - y1)) / (y2 - y1);
        draw_line(start_x, y, end_x, y, color);
    }
}

void draw_sand(uint16_t progress)
{
    int fill_height = (progress * 95) / 1023; // Scale progress
    fill_trapezoid(130, 120, 190, 120, 230, 190, 90, 190, 0xFFE0);
    for (int y = 120; y < 120 + fill_height; y++)
    {
        draw_line(160 - (y - 120), y, 160 + (y - 120), y, 0xFFE0); // Yellow sand filling
    }
}

void get_hourglass_bounds(int y, int *x_left, int *x_right)
{
    if (y < 120)
    { // Upper part of the hourglass
        *x_left = 90 + ((155 - 90) * (y - 50)) / (120 - 50);
        *x_right = 230 + ((165 - 230) * (y - 50)) / (120 - 50);
    }
    else
    { // Lower part of the hourglass
        *x_left = 155 + ((90 - 155) * (y - 120)) / (190 - 120);
        *x_right = 165 + ((230 - 165) * (y - 120)) / (190 - 120);
    }
}

int main()
{
    volatile int *pixel_ctrl_ptr = (int *)0xFF203020;
    pixel_buffer_start = *pixel_ctrl_ptr;
    // volatile uint16_t *switches = (uint16_t *)SWITCHES_BASE;

    clear_screen();

    // if (!hourglass_drawn) {
    //     draw_hourglass_frame();
    //     hourglass_drawn = true;
    // }

    draw_hourglass_frame();

    // int y_trap = 50;
    int x_trap_left, x_trap_right;

    while (1)
    {
        for (int y_trap = 50; y_trap <= 120; y_trap++)
        { // Using simple increment for smoother filling
			clear_screen();
			draw_hourglass_frame();
            get_hourglass_bounds(y_trap, &x_trap_left, &x_trap_right);
            fill_trapezoid(x_trap_left, y_trap, x_trap_right, y_trap, 165, 120, 155, 120, 0xFFE0);
            wait_for_v_sync();			
        }
    }

    // while (1) {
    //     uint16_t progress = *switches & 0x3FF;
    //     draw_sand(progress);
    //     wait_for_v_sync();
    // }
    return 0;
}
