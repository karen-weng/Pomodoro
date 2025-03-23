#include <stdbool.h>
#include <math.h>
void plot_pixel(int x, int y, short int line_color); // plots one pixel
void clear_screen(); // clears whole screen
void swap(int *num1, int *num2);
void draw_line(int x0,int  y0,int x1,int y1, short int colour);
void wait_for_v_sync();
void draw_box(int x, int y, short int colour) ;


int pixel_buffer_start; // global variable
short int Buffer1[240][512]; // 240 rows, 512 (320 + padding) columns
short int Buffer2[240][512];

int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    // declare other variables(not shown)
    // initialize location and direction of rectangles(not shown)

    // 319, 239

    int x_coor_array[8];
    int y_coor_array[8];
    int x_velocity[8];
    int y_velocity[8];

    for (int i = 0; i < 8; i++) {
        x_coor_array[i] = rand() % (318) + 1;
        y_coor_array[i] = rand() % 238 + 1;
        x_velocity[i] = rand() % 7 - 3;
        y_velocity[i] = rand() % 7 - 3;
    }

    /* set front pixel buffer to Buffer 1 */
    *(pixel_ctrl_ptr + 1) = (int) &Buffer1; // first store the address in the  back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_v_sync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); // pixel_buffer_start points to the pixel buffer

    /* set back pixel buffer to Buffer 2 */
    *(pixel_ctrl_ptr + 1) = (int) &Buffer2;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
    clear_screen(); // pixel_buffer_start points to the pixel buffer

    while (1) {
        wait_for_v_sync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
        clear_screen();
        for (int i = 0; i < 8; i++) {
            if (x_coor_array[i]-1 <= 0 || x_coor_array[i]+1 >= 319) {
                x_velocity[i] *= -1;
            }
            if (y_coor_array[i]-1 <= 0 || y_coor_array[i]+1 >= 239) {
                y_velocity[i] *= -1;
            }            

            x_coor_array[i] += x_velocity[i];
            y_coor_array[i] += y_velocity[i];

            draw_box(x_coor_array[i], y_coor_array[i], 0x07E0);
            
            if (i==7) {
                draw_line(x_coor_array[i], y_coor_array[i], x_coor_array[0], y_coor_array[0], 0x07E0);
            }
            else {
                draw_line(x_coor_array[i], y_coor_array[i], x_coor_array[i+1], y_coor_array[i+1], 0x07E0);
            }
        }
    }
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

void wait_for_v_sync() {
    volatile int* fbuf= (int*) 0xFF203020;
    int status;
    *fbuf = 1;
    status = *(fbuf + 3);
    while ((status & 0x01) != 0) {
        status = *(fbuf + 3); 
    }
}

void draw_box(int x, int y, short int colour) {
    plot_pixel(x-1, y-1, colour);
    plot_pixel(x, y-1, colour);
    plot_pixel(x+1, y-1, colour);
    plot_pixel(x-1, y, colour);
    plot_pixel(x, y, colour);
    plot_pixel(x+1, y, colour);
    plot_pixel(x-1, y+1, colour);
    plot_pixel(x, y+1, colour);
    plot_pixel(x+1, y+1, colour);
}


