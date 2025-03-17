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
	la s1, sec_countdown
	sw s0, (s1)	# s1 is sec countdown

    la s5, LEDs
    

idle:   # keep looping till interrupt happens
    la s1, sec_countdown
	lw s2, (s1)
    sw s2, (s5)
    j idle


timer_interrupt_handler:
    addi sp, sp, -24
    sw ra, 0(sp)
    sw s0, 4(sp)
    sw s1, 8(sp)
    sw t0, 12(sp)
    sw t1, 16(sp)
    
    li t0, TIMER_IRQ
    csrr t1, mcause # check what was pressed (IRQ 16/18)
    andi t1, t1, 0x7ff
    bne t1, t0, key_interrupt_handler
    # interrupt is from timer, decrease countdown
    la s0, sec_countdown
    lw s1, (s0)
    addi s1, s1, -1
    bnez s1, skipCountdownReset

skipCountdownReset:
    sw s1, (s0)
    la s0, TIMER
    sw x0, T_STATUS(s0)

end_interrupt:
    lw ra, 0(sp)
    lw s0, 4(sp)
    lw s1, 8(sp)
    lw t0, 12(sp)
    lw t1, 16(sp)

    addi sp, sp, 24

    mret    

key_interrupt_handler:
    j end_interrupt

config_timer:
    la t0, TIMER
    li t1, COUNTER_START_VAL
    sw t1, T_COUNTER_START_LOW(t0)   # store initial counter start
    srli t1, t1, 16
    sw t1, T_COUNTER_START_HIGH(t0)
    sw x0, T_STATUS(t0) # reset timeout (TO) signal
    li t1, 0b111
    sw t1, T_CONTROL(t0)    # set up interrupts (ITO) & continuous operation of timer (CONT)
    ret

config_keys:
    la t0, KEYS
	li t1, 0xF
    sw t1, K_EDGE(t0)   # reset edge bits
    sw x0, K_INTERRUPT(t0)
    li t1, 0b111
    sw t1, K_INTERRUPT(t0)   # set up interruptmask to enabl keys 0-2
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
sec_countdown: .word 0
min_countdown: .word 0