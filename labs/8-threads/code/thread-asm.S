/*
 * implement these.  ordered in difficulty.
 */
#include "rpi-asm.h"

#define NOTREACHED mov r0, pc; bl asm_not_reached
#define UNIMPL mov r0, pc; bl asm_not_implemented

@ return the current stack pointer.
MK_FN(rpi_get_sp)
    UNIMPL
	bx lr

MK_FN(rpi_cswitch)
	@ push the registers and save the stack pointer.
	push {r4-r11, lr}
	str sp, [r0, #0]

	mov sp, r1  @r1 has new sp in it

	@get thread we are switching to's saved registers
	pop {r4-r11, lr}
	
    bx lr

@ [Make sure you can answer: why do we need to do this?]
@
@ use this to setup each thread for the first time.
@ setup the stack so that when cswitch runs it will:
@	- load address of <rpi_init_trampoline> into LR
@	- <code> into r1, 
@	- <arg> into r0
@ 
MK_FN(rpi_init_trampoline)
	mov r0, r5 	@ move the argument for the thread's func to its arg1 register
	blx r4 		@ branch and link to code thats at address in r4
	bl rpi_exit @ call rpi_exit to avoid missing them.