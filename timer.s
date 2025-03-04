.equ LED_BASE			    0xFF200000
.equ HEX3_HEX0_BASE			0xFF200020
.equ HEX5_HEX4_BASE			0xFF200030
.equ SW_BASE				0xFF200040
.equ KEY_BASE			0xFF200050
.equ TIMER_BASE			0xFF202000
# .equ AUDIO_BASE			0xFF203040


.global
_start:
la s0, SW_BASE
la s1, KEY_BASE
la s2, TIMER_BASE
la s3, LED_BASE



la sp, 0x20000

#interrupts
csrw mstatus, x0

setup_key:
    li t0, 0xF
    sw t0, 8(s1)
    sw t0, 12(s1)

setup_timer:
    li t0, 12000000 #25 min
    # ^probably want to change this line to memory in the future
    lw t0, 8(s2)
    srli t0, t0, 16
    lw t0, 12(s2)
    li t0, 0b1001
    lw t0, 0(s0)

#cpu allow interupts form
li t0, 0x50000 #key and timer
csrw mie, t0

li t0, interrupt_handler 
csrw mie, t0



interrupt_handler:
#stack

    lw t0, mcause
    andi t1, t0, 0x40000
    bgtz t1, key_interrupt
    andi t1, t0, 0x10000
    bgtz t1, timer_interrupt
    ret

key_interrupt:
    lw t0, 12(s1) #load edge
    li t1, 0xF
    sw t1, 12(s1) #reset edge
    li t1, 1
    andi t2, t1, t0
    bgtz t2, key0

    slli t1, 1
    andi t2, t1, t0
    bgtz t2, key1

    slli t1, 1
    andi t2, t1, t0
    bgtz t2, key2

    slli t1, 1
    andi t2, t1, t0
    bgtz t2, key3

    ret

key0:
#set/start timer
key1:
#pause/start timer
key2:
#reset timer
key3:


timer_interrupt:




