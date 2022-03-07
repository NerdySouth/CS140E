#include "rpi-asm.h"

@ just use the default int stack address.
#define INT_STACK INT_STACK_ADDR

#define not_reached()  mov r0, pc; b asm_not_reached

@ create an interrupt vector named <name>
@ user has to define the different trampolines.
#define MK_INT_VEC(name)    \
    .align 5;               \
    .globl name;            \
    name:                   \
        b reset;            \
        b undef;            \
        b swi_handler;      \
        b prefetch_abort;   \
        b data_abort;       \
        b reset;            \
        b interrupt

@ save all the stuff we need, then call the given routine.
#define TRAMPOLINE(fn)              \
  mov sp, #INT_STACK;               \
  push  {r0-r12,lr};                \
  mov   r0, lr;                     \
  bl    fn;                         \
  pop   {r0-r12,lr};                \
  movs    pc, lr


// keep the name of the trampoline private.
#define MK_TRAMPOLINE(name, offset, fn)  \
    name: sub   lr, lr, # (offset); TRAMPOLINE(fn)


@ call the default handlers and blow up.  override if you
@ want to do something fancier.
MK_TRAMPOLINE(reset,            4, reset_vector)
MK_TRAMPOLINE(undef,            4, undefined_instruction_vector)
MK_TRAMPOLINE(prefetch_abort,   4, prefetch_abort_vector)
MK_TRAMPOLINE(data_abort,       8, data_abort_vector)
MK_TRAMPOLINE(interrupt,        4, int_vector)


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ part0/part1: just handle system calls.
@

swi_handler:
    @ when we do equiv, need to make sure we restore everything back.
    @ we over-save/restore and trim after equiv.
    push {r1-r12,r14}
    bl do_syscall
    pop {r1-r12,r14}
    movs pc, lr

.align 5; 
.globl part1_swi_vec
part1_swi_vec: 
    b reset
    b undef
    b swi_handler
    b prefetch_abort
    b data_abort
    b reset
    b interrupt


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ part2: handle single stepping
@

prefetch_handler_single_step:
    sub   lr, lr, #4
    TRAMPOLINE(prefetch_abort_single_step)

@ have a single step handler for prefetch abort.
.align 5; 
.globl part2_single_step_vec
part2_single_step_vec: 
    b reset
    b undef
    b swi_handler
    b prefetch_handler_single_step
    b data_abort
    b reset
    b interrupt



@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ part3: do equivalance for pc

@ setup and call prefetch_abort_equiv_pc (look at the single step)
prefetch_handler_equiv_pc:
    sub lr, lr, #4
	TRAMPOLINE(prefetch_abort_equiv_pc)

@ have a single step handler for prefetch abort.
@ have a equiv handler for prefetch abort.
.align 5; 
.globl part3_equiv_pc_vec
part3_equiv_pc_vec: 
    b reset
    b undef
    b swi_handler
    b prefetch_handler_equiv_pc
    b data_abort
    b reset
    b interrupt


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ part4: do equivalance for all regs
@   push all the regs and spsr onto the stack and pass it to the handler.


@ setup and call prefetch_abort_equiv
prefetch_handler_equiv:
	sub  lr, lr, #4
	mov sp, #INT_STACK

	@ push registers
	sub sp, sp, #68 	@ make room for array of regs
	stm sp, {r0-r14}^	@ push registers to stack
	str lr, [sp, #60]	@ push val of lr 
	mrs r1, spsr		@ push spsr to arg 2
	str r1, [sp, #64]	@ store spsr in reg arra]
	mov r0, sp			@ store start of array in arg 1
	mov r2, lr			@ store lr in arg 3

	bl prefetch_abort_equiv	@ enter handler

	pop {lr} 
	ldm sp, {r0-r14}^	@ load all regs back from stack
	movs pc, lr 		@ return control


@ have a single step handler for prefetch abort.
@ have a equiv handler for prefetch abort.
.align 5; 
.globl part4_equiv_vec
part4_equiv_vec: 
    b reset
    b undef
    b swi_handler
    b prefetch_handler_equiv
    b data_abort
    b reset
    b interrupt

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ part5: save to process structure
@   push all the regs and spsr onto the stack and pass it to the handler.


@ setup and call prefetch_abort_equiv

prefetch_handler_equiv_struct:
	sub  lr, lr, #4

	@ load in the curr process global
	ldr sp, =pix_cur_process
	ldr sp, [sp]

	@ push registers to curr process struct
	stm sp, {r0-r14}^	@ push registers to stack
	str lr, [sp, #60]	@ push val of lr 
	mrs r1, spsr		@ push spsr to arg 2
	str r1, [sp, #64]	@ store spsr in reg arra]
	mov r0, sp			@ store start of array in arg 1
	mov r2, lr			@ store lr in arg 3

	@ setup sp for function
	mov sp, #INT_STACK	

	bl prefetch_abort_equiv_struct	@ enter handler

	ldr sp, =pix_cur_process	
	ldr sp, [sp]
	
	ldm sp, {r0-r14}^	@ load all regs back from stack
	ldr lr,  [sp, #60]
	movs pc, lr 		@ return control


@ have a single step handler for prefetch abort.
@ have a equiv handler for prefetch abort.
.align 5; 
.globl part5_equiv_vec
part5_equiv_vec: 
    b reset
    b undef
    b swi_handler
    b prefetch_handler_equiv_struct
    b data_abort
    b reset
    b interrupt


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ before jumping to user:
@  - clear all bits in cpsr besides mode
@  - clear all registers before jumping to user space besides sp
@
@ does not return.
MK_FN(user_mode_run_fn)
    @ use cps to switch to user level.
    @ make sure to do a "prefetch flush" to 
    @ flush the state  (see rpi-asm.h)
    @
    @ for later, you should clear all the registers.
	
	@change to user mode
	cps #16

	@prefetch flush
	prefetch_flush(r2)

	@ clear cpsr bits
	msr cpsr_cxsf, r2

	@clear all registers besides sp (r13) and two argmuents to this func (in r0, r1) 
	mov r2, #0
	mov r3, #0 
	mov r4, #0 
	mov r5, #0 
	mov r6, #0 
	mov r7, #0 
	mov r8, #0 
	mov r9, #0 
	mov r10, #0 
	mov r11, #0 
	mov r12, #0 
	mov r14, #0 

	@ setup sp to point to right place (addr found in r1)
	mov r13, r1

	@branch to user code (first arg; r0), use bx so that we dont load in anything for lr (r14)
	bx r0

@ c func: switchto_asm(&proc->reg_save[0])
MK_FN(switchto_asm)
	mov sp, r0
	ldm sp, {r0-r14}^	@ load all regs back from stack
	ldr lr, [sp, #60]
	add sp, sp, #60
	rfe sp!