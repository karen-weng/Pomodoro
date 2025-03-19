#include <stdio.h>
#include <stdbool.h>

struct keyboard_press_t
{
bool A;
bool S;
bool D;
bool W;
};

struct keyboard_t
{
    volatile unsigned char data;
    volatile unsigned char RVALID;
    volatile unsigned short RAVAIL;
    volatile unsigned int PS2_Control;
};

struct keyboard_t* const keyboard = (struct keyboard_t*) 0xFF200100;
volatile int* LEDs = (int*) 0xff200000;

struct keyboard_press_t keyboard_press;

void read_PS2();
void set_PS2();
void i_setup();
static void ihandler() __attribute__ ((interrupt ("machine")));

int main()
{
    i_setup();
    set_PS2();

    while (1)
{
int to_display = 0;
if (keyboard_press.W)
to_display += 0x8;
if (keyboard_press.S)
to_display += 0x4;
if (keyboard_press.A)
to_display += 0x2;
if (keyboard_press.D)
to_display += 0x1;
*LEDs = to_display;
}

}

void read_PS2()
{
    char read_data = keyboard->data;
if (read_data == 0xF0)
read_data = keyboard->data;
if (read_data == 0x1C)
keyboard_press.A = !keyboard_press.A;
if (read_data == 0x1B)
keyboard_press.S = !keyboard_press.S;
if (read_data == 0x1D)
keyboard_press.W = !keyboard_press.W;
if (read_data == 0x23)
keyboard_press.D = !keyboard_press.D;
}

void set_PS2()
{
    while ((keyboard->RAVAIL) & 0xF) { keyboard->data; } //empty out fifo
    keyboard->PS2_Control = 1; //enable interrupts
}

void i_setup()
{
    int mstatus_value, mtvec_value, mie_value;
    mstatus_value = 0b1000;
    __asm__ volatile ("csrc mstatus, %0" :: "r"(mstatus_value)); //clear mie enable
    mtvec_value = (int) &ihandler;
    __asm__ volatile ("csrw mtvec, %0" :: "r"(mtvec_value)); //set handler address
    __asm__ volatile ("csrr %0, mie" : "=r"(mie_value));
    __asm__ volatile ("csrc mie, %0" :: "r"(mie_value)); //clear all current IRQ
    mie_value = 0x400000;
    __asm__ volatile ("csrs mie, %0" :: "r"(mie_value)); //set IRQ
    __asm__ volatile ("csrs mstatus, %0" :: "r"(mstatus_value)); //enable mie
}

void ihandler() {
    int mcause_value;
    __asm__ volatile ("csrr %0, mcause" : "=r"(mcause_value));
    if (mcause_value == 0x80000016) //IRQ = 22
        read_PS2();
}