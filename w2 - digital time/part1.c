#include <stdbool.h>
#include <math.h>
void plot_pixel(int x, int y, short int line_color); // plots one pixel
void clear_screen(); // clears whole screen
void swap(int *num1, int *num2);
void draw_line(int x0,int  y0,int x1,int y1, short int colour);

int pixel_buffer_start; // global variable

int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    /* Read location of the pixel buffer from the pixel buffer controller */
    pixel_buffer_start = *pixel_ctrl_ptr;

    clear_screen();
    draw_line(0, 0, 150, 150, 0x001F);   // this line is blue
    draw_line(150, 150, 319, 0, 0x07E0); // this line is green
    draw_line(0, 239, 319, 239, 0xF800); // this line is red
    draw_line(319, 0, 0, 239, 0xF81F);   // this line is a pink color
}


void clear_screen()
{
    int y, x;
    for (x = 0; x < 320; x++)
    for (y = 0; y < 240; y++)
    plot_pixel (x, y, 0);
}


void plot_pixel(int x, int y, short int line_color)
{
    volatile short int *one_pixel_address;

        one_pixel_address = pixel_buffer_start + (y << 10) + (x << 1);

        *one_pixel_address = line_color;
}

// 1 draw_line(x0, y0, x1, y1)

void swap(int *num1, int *num2) {
    int temp = *num1;
    *num1 = *num2;
    *num2 = temp;
}

void draw_line(int x0,int  y0,int x1,int y1, short int colour) {

    bool is_steep = abs(y1 - y0) > abs(x1 - x0);
    if (is_steep) {
        swap(&x0, &y0);
        swap(&x1, &y1);
    }

    if (x0 > x1) {
        swap(&x0, &x1);
        swap(&y0, &y1);
    }

    int deltax = x1 - x0;
    int deltay = abs(y1 - y0);
    int error = -(deltax / 2);
    int y = y0;
    int y_step;

    if (y0 < y1) {
        y_step = 1; 
    }
    else {
        y_step = -1;
    }

    for (int x = x0; x < x1; x++) {
        if (is_steep) {
            plot_pixel(y, x, colour);
        }
        else {
            plot_pixel(x, y, colour);
        }
        error = error + deltay;
        if (error > 0) {
            y = y + y_step;
            error = error - deltax;
        }
    }
    return;
}


