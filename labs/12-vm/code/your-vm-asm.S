@ Must carefully read B2 of the armv6 manual.  You cannot rely on "well it
@ worked on my test case": mistakes cause stale data, which may or may not
@ show up in your test case (likely not) despite being wildly broken.
@
@ Note: Rd is allegedly a read-only parameter to these instructions, but
@ the manual says SBZ (should be zero).  I think (and the linux source seems
@ to think) that this means we have to explicitly set it to 0.  Not setting
@ Rd=0 is an easy mistake to make.  Perhaps is worth building in (Linux does
@ not, but) --- the cost of an extra mov is negligible given our other 
@ overheads.
@
@ Alot of people put these instructions in inline assembly, wrapped up by 
@ function calls (e.g., cs107e's MMU code).  This practice is dangerous. 
@ For example, if you are flushing caches, you cannot be sure that the 
@ function return, etc does not then reload them, esp w.r.t. the BTB.  I 
@ think the only safe approach is to put directly in assembly so you are 
@ guaranteed no branch, indirect jump, load/store happens in between.
@
@ A lot of MMU code out there appears pretty broken b/c they don't do enough
@ flushing operations and/or rely on gcc code generation to not do the above.

#include "rpi-asm.h"
#include "armv6-coprocessor-asm.h"

#define UNIMPL mov r0, lr; b asm_not_implemented

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ many page numbers are in 
@    armv6-coprocessor-asm.h
@ and the other armv6-*.h files

// just read the cache type: use C code to figure out if it is unified
// b6-13, b6-14
MK_FN(get_cache_type)
    UNIMPL
    bx lr

// void reset(void)
// clear the dcache, icache, tlbs
MK_FN(mmu_reset)

	@  see armv6 B6-B7, or memory order cheatsheet for nuance of obvserving results of maintenance ops
	mov r0, #0, 0
	INV_ALL_CACHES(r0) 				@ invalidate all caches (D/I caches)
	DSB(r0)							@ observe completion of above
	INV_TLB(r0)						@ clear TLB
	DSB(r0)							@ observe completion of above
	PREFETCH_FLUSH(r0)				@ let otherrs observe completion of TLB clear
	FLUSH_BTB(r0)					@ clear out branch predition (BTB)
	DSB(r0)							@ DSB so above completes
	PREFETCH_FLUSH(r0)				@ let others observe completion of BTB Flush

    bx lr

// void cp15_domain_ctrl_wr(uint32_t dom_reg);
//  need to do cp15 sync: flush_btb, dsb, prefetch flush
MK_FN(cp15_domain_ctrl_wr)
	mov r1 , #0, 0				@ clear r1, we will use it later as a prefect flush
	DOMAIN_CTRL_WR(r0)			@ write to the Domain Access & Control Register with val in r0: see armv6 b2-24
	PREFETCH_FLUSH(r1)			@ prefetch flush so we observe changes to DACR above: see armv6 b6-21 for what it does
	@ DOMAIN_CTRL_WR(r0)
	@ DSB(r1);
	@ FLUSH_BTB(r1)
	@ DSB(r1)
	@ PREFETCH_FLUSH(r1)
    bx lr

// void cp15_set_procid_ttbr0(uint32_t proc_and_asid, fld_t *pt);
// sequence from b2-25
MK_FN(cp15_set_procid_ttbr0)
	mov r2, #0, 0	 		@ clear r2 so we have a reg to use for prefetch flush later

	@ Do the dance ARM lays out: see armv6 b2-25
	ASID_SET(r2)			@ set ASID to 0
	PREFETCH_FLUSH(r2)		@ prefetch flush
	TTBR0_SET(r1)			@ set the base addr in ttbr0 to addr of page table
	DSB(r2)					@ dsb since writing with new reg
	TTBR1_SET(r2)			@ set ttbr1 to 0 (not using)
	DSB(r2)					@ dsb
	PREFETCH_FLUSH(r2)		@ prefetch flush
	ASID_SET(r0)			@ set new ASID
	PREFETCH_FLUSH(r2)

	@ need to flush BTB see armv6 b2-24 2.7.5
	FLUSH_BTB(r2)
	DSB(r2)			@ need DSB so we know FLUSH completed, but need the prefetch_flush to complete it
	PREFETCH_FLUSH(r2)

    bx lr

// void mmu_disable_set_asm(cp15_ctrl_reg1_t c);
MK_FN(mmu_disable_set_asm)

    @ note: this piece of code is mapped into the sample place in both
    @ MMU-enabled and MMU-disabled so BTB and Prefetch entries work identically
    @ to both MMU-enable and disable.   Without this requirement I believe is
    @ very hard to get right (perhaps impossible?) since we cannot flush
    @ the BTB and prefetch and switch MMU on/off atomically.  thus, would
    @ flush, but instructions could get fetched before we could disable,
    @ defeating the purpose.

	mov r1, #0, 0	
	@ disable the MMU. See armv6 6.4.1-6.4.2 on enable/disable mmu
	CLEAN_INV_DCACHE(r1)		@ want to clean data cache & invalidate the data cache
	DSB(r1)						@ want completion of previous clean/invalidate
	CONTROL_REG1_RD(r2)			@ get control reg value
	bic r2, r2, #4, 0			@ clear bits in control reg. See armv6 6.4.2
	CONTROL_REG1_WR(r2)			@ write new val to control reg
	DSB(r2)						@ dsd for completion of write to control reg
	CONTROL_REG1_WR(r0)			@ write cleared value to control reg c
	PREFETCH_FLUSH(r1)			@ prefetch flush to complete coprocessor operation
	INV_ICACHE(r1)				@ invalidate ICACHE
	DSB(r1)						@ want complpetion of previous invalidation
	FLUSH_BTB(r1)				@ flush branch predition
	DSB(r1)						@ observe completion of flush operation
	PREFETCH_FLUSH(r1)			@ prefetch flush to complete BTB flush
    bx lr

// void mmu_enable_set_asm(cp15_ctrl_reg1_t c);
MK_FN(mmu_enable_set_asm)
	mov r1, #0, 0

	@ enable the MMU. See armv6 6.4.1-6.4.2 on enable/disable mmu
	CONTROL_REG1_WR(r0)		@ setup control reg c (passed into r0)
	DSB(r1)					@ need a dsb to observe the completion of last task
	FLUSH_BTB(r1)			@ flush the BTB to invalidate. Could have garbage in it
	DSB(r1)					@ need dsb to observe completion of last task
	PREFETCH_FLUSH(r1)		@ need PREFETCH_FLUSH to finish flush of BTB
    bx lr