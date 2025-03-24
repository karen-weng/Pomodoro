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

#define clock_rate 100000000
#define quarter_clock clock_rate / 4

static void handler(void) __attribute__((interrupt("machine")));
void set_itimer(void);
void set_KEY(void);
void itimer_ISR(void);
void KEY_ISR(void);

void set_PS2();

volatile int *PS2_ptr = (int *)0xFF200100; // PS/2 port address
int makeNumbers[] = {0x45, 0x16, 0x1E, 0x26, 0x25, 0x2E, 0x36, 0x3D, 0x3E, 0x46};
int makeArrows[] = {0x75, 0x6B, 0x72, 0x74}; // up, left, down, right
int makeOther[] = {0x5A, 0x29, 0x66};        // enter, space, backspace
int makeOtherE0[] = {0x05, 0x06, 0x04};      // F1, F2, F3
volatile unsigned char PS2_data;
volatile unsigned char RVALID;

volatile unsigned char byte1 = 0;
volatile unsigned char byte2 = 0;
volatile unsigned char byte3 = 0;

volatile int pom_start_val = 25;
volatile int small_break_start_val = 5;
volatile int big_break_start_val = 15;
volatile int sec_time = 0;
volatile int min_time = 0;
volatile int key_mode = 1;       // 1 for start, 2 for pause, 3 for stop (ringing) --> multiply by 2 for break vals
volatile bool study_mode = true; // 0 for pomodoro, 1 for break, 2 for long break
volatile int study_session_count = 1;

volatile int *LEDR_ptr = (int *)LEDR_BASE;
volatile int *HEX3_HEX0_ptr = (int *)HEX3_HEX0_BASE;
volatile int *TIMER_ptr = (int *)TIMER_BASE;
volatile int *KEY_ptr = (int *)KEY_BASE;

volatile int led_display_val = 0;

int main(void)
{
    /* Declare volatile pointers to I/O registers (volatile means that the
     * accesses will always go to the memory (I/O) address */

    set_itimer();
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
    mie_value = 0x450000; // KEY, itimer, ps/2 port (non-dual) -- irq 16, 18, 22
    // set interrupt enables
    __asm__ volatile("csrs mie, %0" ::"r"(mie_value));
    // enable Nios V interrupts
    __asm__ volatile("csrs mstatus, %0" ::"r"(mstatus_value));

    sec_time = pom_start_val;

    while (1)
    {
        // *LEDR_ptr = sec_time;
        *LEDR_ptr = led_display_val;
    }
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
    else if (mcause_value == 0x80000012) // KEY port
        KEY_ISR();

    else if (mcause_value == 0x80000016)
    { // IRQ = 22
        led_display_val = 0;
        // int timeout = 0; // Set a timeout value to prevent infinite loop
        // while ((*PS2_ptr & 0x8000))
        // {                        // While RVALID is set and timeout not reached
            PS2_data = *PS2_ptr; // Read data and implicitly decrement RAVAIL

            // Update byte history
            byte1 = byte2;
            byte2 = byte3;
            byte3 = PS2_data & 0xFF;
            
            // if (byte1 == 0xE0)
            // {
            //     led_display_val = 64;
            // }
            // if (byte2 == 0xE0)
            // {
            //     led_display_val = 32;
            // }
            // if (byte3 == 0xE0)
            // {
            //     led_display_val = 16;
            // }

            // Minimal processing - just light LED for arrow keys
            if (byte1 == 0xE0 && byte2 == 0xF0)
            {

                switch (byte3)
                {
                case 0x75:
                    led_display_val = 1;
                    break; // Up
                case 0x6B:
                    led_display_val = 2;
                    break; // Left
                case 0x72:
                    led_display_val = 4;
                    break; // Down
                case 0x74:
                    led_display_val = 8;
                    break; // Right
                }
            }
            // timeout++; // increment timeout counter
        // }
    }
    // else, ignore the trap
    else
    {
        printf("Unexpected interrupt at %d.", mcause_value);
    }
}

// FPGA interval timer interrupt service routine
void itimer_ISR(void)
{
    *TIMER_ptr = 0; // clear interrupt
    sec_time -= 1;  // decrease pom timer counter
    if (sec_time == 0)
    { // if pom timer done, on 'stop' mode
        key_mode = 3;
        *(TIMER_ptr + 0x1) = 0xB; // 0b1011 (stop, cont, ito)
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
            study_mode = !study_mode;
            study_session_count++;
            if (study_mode)
            {
                sec_time = pom_start_val;
            }
            else if (!study_mode && study_session_count % 4 != 0)
            {
                sec_time = small_break_start_val;
            }
            else if (!study_mode)
            {
                sec_time = big_break_start_val;
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
    {
    }
    else
    {
        printf("Unexpected key %d pressed.", pressed_key);
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

// Configure the KEY port
void set_KEY(void)
{
    volatile int *KEY_ptr = (int *)KEY_BASE;
    *(KEY_ptr + 3) = 0xF; // clear EdgeCapture register
    *(KEY_ptr + 2) = 0x3; // enable interrupts for keys 0-1
}

void set_PS2()
{
    // Keep reading while RVALID (bit 15) is set
    while (*PS2_ptr & 0x8000)
    {
        PS2_data = *PS2_ptr; // Read and discard
    }

    *(PS2_ptr + 1) = 1; // enable interrupts RE bit
}