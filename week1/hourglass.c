#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#define SWITCHES_BASE 0xFF200040     // Memory-mapped address for switches
#define PIXEL_BUFFER_BASE 0xFF203020 // Memory-mapped address for pixel buffer

// volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUFFER_BASE;
int pixel_buffer_start;
short int Buffer1[240][512]; // 240 rows, 512 (320 + padding) columns
short int Buffer2[240][512];
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

void draw_hourglass_frame_top()
{
    draw_line(90, 50, 230, 50, 0xF800);   // Top horizontal line
    draw_line(90, 50, 155, 120, 0xF800);  // Left diagonal down
    draw_line(230, 50, 165, 120, 0xF800); // Right diagonal down
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

void draw_initial_yellow_trapezoid()
{
    fill_trapezoid(90, 50, 230, 50, 165, 120, 155, 120, 0xFFE0); // Draw once
}

void erase_sand_sliver(int y)
{
    int x_left, x_right;
    // get_hourglass_bounds(y, &x_left, &x_right);
    // Clear the current sand row by drawing the background color (0x0)
    draw_line(90, y - 2, 230, y - 2, 0x0);
    draw_line(90, y - 1, 230, y - 1, 0x0);
    draw_line(90, y, 230, y, 0x0);
}

int main()
{
    volatile int *pixel_ctrl_ptr = (int *)0xFF203020;
    
    // Animation timing configuration
    const int ANIMATION_TIME_SECONDS = 30;        // Total animation duration in seconds
    const int VSYNC_FREQUENCY = 60;               // VSync frequency in Hz (60 FPS)
    const int TOTAL_FRAMES = ANIMATION_TIME_SECONDS * VSYNC_FREQUENCY;
    
    // Calculate how many VSync events should occur between sand level updates
    const int SAND_HEIGHT_CHANGE = 70;            // Total pixels the sand level changes (120-50)
    const int VSYNCS_PER_UPDATE = TOTAL_FRAMES / SAND_HEIGHT_CHANGE;
    
    // Set up double buffering
    *(pixel_ctrl_ptr + 1) = (int)&Buffer1;        // Set back buffer to Buffer1
    wait_for_v_sync();                            // Wait for swap
    pixel_buffer_start = *pixel_ctrl_ptr;         // Get front buffer address
    clear_screen();                               // Clear front buffer

    draw_hourglass_frame();                       // Draw initial frame

    // Set back buffer to Buffer2
    *(pixel_ctrl_ptr + 1) = (int)&Buffer2;

    // Draw frame in back buffer
    draw_hourglass_frame();

    // Variables to track sand levels
    int top_level = 50;                           // Start with top part full
    int triangle_height = 0;                      // Height of the bottom triangle (0 at start)
    int triangle_width = 0;                       // Width of triangle base (0 at start)
    int vsync_counter = 0;                        // Counter for VSync events
    int max_triangle_height = 70;                 // Maximum height of the triangle
    
    // Get hourglass bottom width for triangle base
    int bottom_left, bottom_right;
    get_hourglass_bounds(190, &bottom_left, &bottom_right);
    int max_width = bottom_right - bottom_left;   // Maximum possible width
    
    // Bottom triangle properties
    int base_y = 190;                             // Base of triangle is at the bottom of hourglass
    int center_x = (bottom_left + bottom_right) / 2; // Center of hourglass bottom
    
    while (1)
    {
        // Wait for previous frame to finish
        wait_for_v_sync();
        vsync_counter++;

        // Switch to the next back buffer
        pixel_buffer_start = *(pixel_ctrl_ptr + 1);

        // Clear the screen (or just the hourglass area)
        clear_screen();

        // Draw the hourglass frame
        draw_hourglass_frame();

        // Update animation only when enough VSync events have passed
        if (vsync_counter >= VSYNCS_PER_UPDATE) {
            vsync_counter = 0;  // Reset counter
            
            // Update sand levels if animation is not complete
            if (top_level < 120) {
                top_level++;                      // Reduce sand in top
                triangle_height++;                // Increase triangle height
                
                // Increase width proportionally to height
                // This ensures we grow both dimensions at same rate
                triangle_width = (max_width * triangle_height) / max_triangle_height;
            }
        }

        // Reset animation when finished
        if (top_level >= 120 && triangle_height >= max_triangle_height)
        {
            top_level = 50;
            triangle_height = 0;
            triangle_width = 0;
            vsync_counter = 0;
        }

        // Draw sand in top half (from top_level to middle)
        for (int y = top_level; y < 120; y++)
        {
            int x_left, x_right;
            get_hourglass_bounds(y, &x_left, &x_right);
            draw_line(x_left, y, x_right, y, 0xFFE0); // Yellow sand
        }

        // Draw triangle at the bottom
        if (triangle_height > 0) {
            // Calculate the apex y-position based on current triangle height
            int apex_y = base_y - triangle_height;
            
            // Draw each line of the triangle, narrowing as we go up from the base
            for (int y = base_y; y >= apex_y; y--) {
                // Calculate width of this line based on y position
                float progress = (float)(base_y - y) / triangle_height;
                
                // Width narrows from current triangle width to a point at the apex
                int line_width = (int)(triangle_width * (1.0 - progress));
                
                // Center the line in the bottom of the hourglass
                int line_left = center_x - line_width / 2;
                int line_right = center_x + line_width / 2;
                
                // Draw the line for this row of the triangle
                draw_line(line_left, y, line_right, y, 0xFFE0); // Yellow sand
            }
        }
    }

    return 0;
}

