.global _start
_start:

	.equ LEDs,  0xFF200000
	.equ TIMER, 0xFF202000
	.equ CONTROL, 0x04 
	.equ COUNTER_START_LOW, 0x08
	.equ COUNTER_START_HIGH, 0x0C
	
	.equ PUSH_BUTTON, 0xFF200050

	#Set up the stack pointer
	li sp, 0x200000   # initialize the stack
	
	la s6, COUNTER_START

	
	jal    CONFIG_TIMER        # configure the Timer
    jal    CONFIG_KEYS         # configure the KEYs port
	
	/*Enable Interrupts in the NIOS V processor, and set up the address handling
	location to be the interrupt_handler subroutine*/
	# Tell the CPU to accept interrupt requests from IRQ18 when interrupts are enabled
	# set bit 18 of mie to 1
	li t1, 0x40000 #number for 18 of irq
	csrrs x0, mie, t1
	li t1, 0b10000000000000000 # for 16
	csrrs x0, mie, t1
	
	# Tell the CPU to accept interrupts
	# set bit 3 (MIE) of mstatus to 1
	li  a1, 0x8
	csrrs x0, mstatus, a1
	
	la s0, LEDs
	la s1, COUNT
	la s3, RUN
	la s4, PUSH_BUTTON
	la s5, TIMER
	
	LOOP:
	lw s2, 0(s1)          # Get current count
	sw s2, 0(s0)          # Store count in LEDs
	j LOOP


interrupt_handler:
	addi sp, sp, -40
	sw ra, 0(sp)
	sw s0, 4(sp)
	sw s1, 8(sp)
	sw s2, 12(sp)
	sw s3, 16(sp)
	sw t0, 20(sp)
	sw t1, 24(sp)
	sw t2, 28(sp)
	sw t3, 32(sp) 
	sw a0, 36(sp) 
	
	li t0, 18 # Checking if Interrupt comes from IRQ 18 key
	csrr t1, mcause # mcause tells you what was pressed (IRQ 16/18)
	andi t1, t1, 0x7FF # get rid of extra 2 bits
	bne t1, t0, checktimer
	
	# if from key flip run
	lw t0, 12(s4) # s4 is edge, t0 is edge 
    andi t1, t0, 1
    bne t1, x0, key0_action

    srli t0, t0, 1
    andi t1, t0, 1
    bne t1, x0, key1_action

    srli t0, t0, 1
    andi t1, t0, 1
    bne t1, x0, key2_action

	j checktimer # incase interupt is not keys 0-2

key0_action: # if KEY0 is pressed, set binary display to (1)b2
	#for key0
	lw t2, 0(s3)
    xori t2, t2, 1 # flip bit
    sw t2, 0(s3) 
    j interruptContinued

key1_action: # if KEY1 is pressed, increment displayed number till max of (1111)b2
    li t2, 0x8 #enable stop
    sw t2, CONTROL(s5) # set enable bit in the control register

	lw a0, 0(s6)
	srli a0, a0, 1 # halves counter, doubles speed
	li t3, 1562500 # 4 presses
	bgt a0, t3, notMin
	li a0, 1562500
	notMin:

	sw a0, 0(s6)
	call CONFIG_TIMER

    j interruptContinued

key2_action: # if KEY2 is pressed, decrement the number till min of (1)b2
    li t2, 0x8 # enable stop
    sw t2, CONTROL(s5) # set enable bit in the control register

	lw a0, 0(s6)
	slli a0, a0, 1 # doubles counter, halves speed
	li t3, 40000000 # 4 presses
	blt a0, t3, notMax
	li a0, 40000000
	notMax:

	sw a0, 0(s6)
	call CONFIG_TIMER

    j interruptContinued


interruptContinued: 

	li t3, 0xF
	sw t3, 12(s4)     # reset EDGE bits
	
checktimer: 
	li t0, 16 # Checking if Interrupt comes from IRQ 16 timer
	csrr t1, mcause
	andi t1, t1, 0x7FF
	bne t1, t0, end_interrupt
	
	#if from timer add to COUNT
	lw t0, 0(s1) #load in count
	lw t1, 0(s3) #load in run
	add t0, t0, t1
	
	li t1, 256
	bne t0, t1, skipCounterReset
	li t0, 0
	skipCounterReset:
	
	sw t0, 0(s1) # store new counter
	sw x0, 0(s5) # store zero in timer to reset TO
		
	end_interrupt:
	
	lw ra, 0(sp)
	lw s0, 4(sp)
	lw s1, 8(sp)
	lw s2, 12(sp)
	lw s3, 16(sp)
	lw t0, 20(sp)
	lw t1, 24(sp)
	lw t2, 28(sp)
	lw t3, 32(sp)
	lw a0, 36(sp)
	
	addi sp, sp, 40

	mret

CONFIG_TIMER: 
#reset tume load number
	la t1, 0xFF202000 #timer
	lw a0, 0(s6)
	mv t0, a0 # load in number to count down from, at start 25000000

	sw x0, 0(t1) # reset TO

    sw t0, COUNTER_START_LOW(t1) # store counter start
	srli t0, t0, 16
	sw t0, COUNTER_START_HIGH(t1)
    li t0, 0x7 #enable start, continue and interupts
    sw t0, CONTROL(t1) # set enable bit in the control register to 1
	
	ret

CONFIG_KEYS: 

	la t0, 0xff200050 # buttons
	li t1, 0xF        # first make sure the EDGE register is all clear
	sw t1, 12(t0)     # reset EDGE bits
	li t1, 0x7        # set MASK bit to enable 0-2
	sw t1, 8(t0) #keys 0-2 buttons
	# CPU SIDE
	# PROCESSOR side:
	# Set the interrupt handler address
	la t1, interrupt_handler
	csrrw x0, mtvec, t1

	ret

.data
/* Global variables */
.global  COUNT
COUNT:  .word    0x0            # used by timer

.global  RUN                    # used by pushbutton KEYs
RUN:    .word    0x1            # initial value to increment COUNT

.global  COUNTER_START          
COUNTER_START:    .word    25000000    # initial value to increment COUNT

.end
