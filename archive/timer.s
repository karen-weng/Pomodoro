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
la s4, study



la sp, 0x20000

#interrupts
csrw mstatus, x0
li a0, 0 #initially study
call setup_key
call setup_timer

setup_key:
    li t0, 0xF
    sw t0, 8(s1)
    sw t0, 12(s1)
    ret

setup_timer:
    #a0 0 if study, a0 4 if break
    and t0, s4, a0
    lw t0, 0(t0) 
    # li t0, 12000000 #25 min
    # ^probably want to change this line to memory in the future
    lw t0, 8(s2)
    srli t0, t0, 16
    lw t0, 12(s2)
    li t0, 0b0101 # start and interrupt
    lw t0, 0(s0)
    ret

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
# 
lw t0, 0(s0) #lw switch value
li t3, 480000

andi t1, t0, 0xF #break time
mul t1, t1, t3
sw t1, 4(s4)

srli t1, t0, 4 #study time
mul t1, t1, t3
sw t1, 0(s4)

beqz a0, break
li a0, 0
call setup_timer

break:
li a0, 4
call setup_timer

key1:
lw t0, 0(s1) 
andi, t0, t0, 2 # run bit check if timer is running or paused
bgtz t0, pause_timer
#timer paused
li t0, 0(s3) 


li t0, 4
j load_pause

pause_timer: # timer current running
li t0, 8

load_pause:
lw t0, 4(s2)
bgtz t0, stop_alarm

stop_alarm:
sw x0, 0(s3) # turn off all leds



#pause/start timer


key2:
#reset timer
call setup_timer

key3:




timer_interrupt:
li t0, -1 
sw t0, 0(s3) #turn on all leds




.data 
study: .word 12000000 #25min
break: .word 2400000 #5min



