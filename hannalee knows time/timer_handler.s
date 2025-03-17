.equ TIMER_IRQ, 16     # processor
.equ KEYS_IRQ, 18

.equ TIMER, 0xFF202000 # timers
.equ T_STATUS, 0x00
.equ T_CONTROL, 0x04
.equ T_COUNTER_START_LOW, 0x08
.equ T_COUNTER_START_HIGH, 0x0C
.equ T_COUNTER_SNAPSHOT_LOW, 0x10
.equ T_COUNTER_SNAPSHOT_HIGH, 0x14
.equ COUNTER_START_VAL, 100000000  # initial value to increment timer COUNT

.equ KEYS, 0xFF200050 # key buttons
.equ K_INTERRUPT, 0x08
.equ K_EDGE, 0x0C

.equ LEDs,  0xFF200000


#.text
.global _start
_start:

    la sp, 0x20000 # enable stack pointer
    csrw mstatus, zero # turn off processor interrupts
    
    call config_timer   # set up timer
    call config_keys    # set up keys
    call config_processor   # set up processor

    # do any non config set up
    li sp, 0x200000 # initialize stack pointer
    la t0, pom_start_val
    lw s0, (t0)
	la s1, sec_time
	sw s0, (s1)	# s1 is sec countdown

    la s5, LEDs
    

idle:   # keep looping till interrupt happens
    la s1, sec_time
	lw s2, (s1)
    sw s2, (s5)
    j idle


timer_interrupt_handler:
    addi sp, sp, -32
    sw ra, 0(sp)
    sw s0, 4(sp)
    sw s1, 8(sp)
	sw s2, 12(sp)
	sw s3, 16(sp)
	sw s4, 20(sp)
    sw t0, 24(sp)
    sw t1, 28(sp)
	sw t2, 32(sp)
    
    li t0, TIMER_IRQ
    csrr t1, mcause # check what was pressed (IRQ 16)
    andi t1, t1, 0x7ff
    bne t1, t0, key_interrupt_handler
    # interrupt is from timer, decrease countdown
    la s0, sec_time
    lw s1, (s0)
    addi s1, s1, -1
	la s2, TIMER
    bnez s1, skipCountdownReset

countdownReset:
	li t0, 0b1011
	sw t0, T_CONTROL(s2)
	la s3, key_mode
	li t1, 3
	sw t1, (s3)

skipCountdownReset:
    sw s1, (s0)
    sw x0, T_STATUS(s2)

end_interrupt:
    lw ra, 0(sp)
    lw s0, 4(sp)
    lw s1, 8(sp)
	lw s2, 12(sp)
	lw s3, 16(sp)
	lw s4, 20(sp)
    lw t0, 24(sp)
    lw t1, 28(sp)
	lw t2, 32(sp)

    addi sp, sp, 32

    mret    

key_interrupt_handler:
	li t0, KEYS_IRQ
    csrr t1, mcause # check what was pressed (IRQ 18)
    andi t1, t1, 0x7ff
	bne t1, t0, problem	# if IRQ is not 16/18, error
	
	la s0, KEYS
	lw t0, K_EDGE(s0) # s4 is edge, t0 is edge 
	la s1, key_mode
	lw s2, (s1)
	la s3, TIMER
    andi t1, t0, 1
    bne t1, x0, key0_start
    srli t0, t0, 1
    andi t1, t0, 1
    bne t1, x0, key1_skip
    j problem

key0_start:	# start/pause/stop key
	li t2, 1
	bne s2, t2, key0_pause
    li t1, 0b111
    sw t1, T_CONTROL(s3)    # set up interrupts (ITO) & continuous operation of timer (CONT)
	li s2, 2
	j key_interrupt
	
key0_pause:
	li t2, 2
	bne s2, t2, key0_stop
	li t0, 0b1011
	sw t0, T_CONTROL(s3)
	li s2, 1
	j key_interrupt
	
key0_stop:
	li t2, 3
	bne s2, t2, problem
    li t1, COUNTER_START_VAL
    sw t1, T_COUNTER_START_LOW(s3)   # store initial counter start
    srli t1, t1, 16
    sw t1, T_COUNTER_START_HIGH(s3)
    sw x0, T_STATUS(s3) # reset timeout (TO) signal
    li t1, 0b11
    sw t1, T_CONTROL(s3)
	li s2, 1
	la t0, pom_start_val
    lw t1, (t0)
	la t2, sec_time
	sw t1, (t2)
	j key_interrupt
	
key1_skip:	# skip key
	j key_interrupt

key_interrupt:
	sw s2, (s1)
	li t0, 0xF
	la t1, KEYS
	sw t0, 12(t1)     # reset EDGE bits
	j end_interrupt

problem:
	j problem

config_timer:
    la t0, TIMER
    li t1, COUNTER_START_VAL
    sw t1, T_COUNTER_START_LOW(t0)   # store initial counter start
    srli t1, t1, 16
    sw t1, T_COUNTER_START_HIGH(t0)
    sw x0, T_STATUS(t0) # reset timeout (TO) signal
    li t1, 0b11
    sw t1, T_CONTROL(t0)    # set up interrupts (ITO) & continuous operation of timer (CONT)
    ret

config_keys:
    la t0, KEYS
	li t1, 0xF
    sw t1, K_EDGE(t0)   # reset edge bits
    sw x0, K_INTERRUPT(t0)
    li t1, 0b11
    sw t1, K_INTERRUPT(t0)   # set up interruptmask to enabl keys 0-1
    ret


config_processor:
    li t0, 1
	slli t0, t0, TIMER_IRQ
	
    csrs mie, t0    # enable irq for timer
	li t0, 1
    slli t0, t0, KEYS_IRQ
    csrs mie, t0    # enable irq for keys
    la t0, timer_interrupt_handler
    csrw mtvec, t0  # provide processor with trap handler address
    li t0, 0b1000
    csrs mstatus, t0    # turn on mie bit to enable processor interrupts
    ret

.data
pom_start_val: .word 25
break_start_val: .word 5
sec_time: .word 0
min_time: .word 0
key_mode: .word 1	# 1 for start, 2 for pause, 3 for stop (ringing) --> multiply by 2 for break vals