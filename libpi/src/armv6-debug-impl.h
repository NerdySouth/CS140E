#ifndef __ARMV6_DEBUG_IMPL_H__
#define __ARMV6_DEBUG_IMPL_H__
#include "armv6-debug.h"


// all your code should go here.  implementation of the debug interface.

//default desired values for bits 14 and 15 of DCSR
#define DSCR_SELECT 14
#define DSCR_ENABLE 15
#define DEBUG_EVENT 0b0010
#define WATCHPOINT_EVENT 0b0010
#define BREAKPOINT_EVENT 0b0001

// example of how to define get and set for status registers
// coproc_mk(status, p14, 0, c0, c1, 0)
coproc_mk(dscr, p14, 0, c0, c1, 0);
coproc_mk_get(dfsr, p15, 0, c5, c0, 0);
coproc_mk_get(wfar, p14, 0, c0, c6, 0);
//watchpoint set
coproc_mk(wvr0, p14, 0, c0, c0, 0b110);
//watchpoint get
coproc_mk(wcr0, p14, 0, c0, c0, 0b111);
coproc_mk(ifsr, p15, 0, c5, c0, 1);
//brkpt get
coproc_mk(bcr0, p14, 0, c0, c0, 0b101);
//brkpt set
coproc_mk(bvr0, p14, 0, c0, c0, 0b100);
coproc_mk(ifar, p15, 0, c6, c0, 2);
coproc_mk(far, p15, 0, c6, c0, 0);




// return 1 if enabled, 0 otherwise.  
//    - we wind up reading the status register a bunch:
//      could return its value instead of 1 (since is 
//      non-zero).
static inline int cp14_is_enabled(void) {
	/* need the value of DSCR to see if it in enabled (bits 14 & 15)
	 * we will do this by reading the register and checking those bits
	 */
	uint32_t dscr_val = cp14_dscr_get();

	if (bit_is_off(dscr_val, 14) && 
		bit_is_on(dscr_val, 15))
		return 1;
	return 0;
}

// enable debug coprocessor 
static inline void cp14_enable(void) {
    // if it's already enabled, just return?
    if(cp14_is_enabled())
        panic("already enabled\n");

    /* for the core to take a debug exception, monitor debug mode has to be both 
     * selected and enabled 
	 * clear bit 14 and set bit 15 of DSCR (Debug Status & Control Register) to enable monitor
	 * mode debugging. Pg. 13-5, 13-7 of arm1176-ch13-debug.pdf
	 */

	// ret =  value of DSCR
	uint32_t dscr_val = cp14_dscr_get(); // -> do we need to do this or can we use 0?

	//clear bit 14
	dscr_val = bit_clr(dscr_val, 14);
	
	//set bit 15
	dscr_val = bit_set(dscr_val, DSCR_ENABLE);

	//write value back to DSCR
	cp14_dscr_set(dscr_val);

    assert(cp14_is_enabled());
}

// disable debug coprocessor
static inline void cp14_disable(void) {
    if(!cp14_is_enabled())
        return;

	 /* for the core to take a debug exception, monitor debug mode has to be both 
     * selected and enabled 
	 * set bit 14 and clear bit 15 of DSCR (Debug Status & Control Register) to disable monitor
	 * mode debugging. Pg. 13-5, 13-7 of arm1176-ch13-debug.pdf
	 */

	// ret =  value of DCSR
	unsigned dscr_val = cp14_dscr_get();

	//set bit 14
	dscr_val = bit_set(dscr_val, DSCR_SELECT);
	
	//clear bit 15
	dscr_val = bit_clr(dscr_val, DSCR_ENABLE);

	// write the new values back to DSCR
	cp14_dscr_set(dscr_val);

    assert(!cp14_is_enabled());
}


static inline int cp14_bcr0_is_enabled(void) {
	unsigned bcr0_val = cp14_bcr0_get();
	if (bit_is_off(bcr0_val, 0))
		return 0;
	return 1;
}
static inline void cp14_bcr0_enable(void) {
	//BCR enable info on  pg. 13-18, 13-19
	unsigned bcr0_val = 0;
	//set enable bit
	bcr0_val = bit_set(bcr0_val, 0);
	//set supervisor access
	bcr0_val = bits_set(bcr0_val, 1, 2, 0b11);
	//set byte address select
	bcr0_val = bits_set(bcr0_val, 5, 8, 0b0001);
	//breakpoint matches in secure & non-secure worlds
	bcr0_val = bits_set(bcr0_val,  14, 15, 0b00); //-> dont need since starting val is 0, these are clear already
	// disable linking
	bcr0_val = bit_clr(bcr0_val, 20); //-> also not needed since already clear
	//set bites 21:22 to 00 so we match on IMVA, not context IDs
	// also dont have to since already clear
	bcr0_val = bits_set(bcr0_val, 21, 22, 0b01);
	cp14_bcr0_set(bcr0_val);
}
//disable bcr0, see pg. 13-18/13-19
static inline void cp14_bcr0_disable(void) {
	if (!cp14_bcr0_is_enabled())
		return;
	//get val of bcr0, clear enable bit, write it back
	unsigned bcr0_val = cp14_bcr0_get();
	bcr0_val = bit_clr(bcr0_val, 0);
	cp14_bcr0_set(bcr0_val);
}


// was this a brkpt fault?
static inline int was_brkpt_fault(void) {
    // use IFSR and then DSCR
	//check ifsr
	unsigned ifsr_val = cp15_ifsr_get();
	if (!bits_eq(ifsr_val, 0, 3, DEBUG_EVENT))
		return 0;
	//check dcsr
	unsigned dscr_val = cp14_dscr_get();
	if (!bits_eq(dscr_val, 2, 5, BREAKPOINT_EVENT))
		return 0;

    return 1;
}

// was watchpoint debug fault caused by a load?
static inline int datafault_from_ld(void) {
    return bit_isset(cp15_dfsr_get(), 11) == 0;
}
// ...  by a store?
static inline int datafault_from_st(void) {
    return !datafault_from_ld();
}


// 13-33: tabl 13-23
static inline int was_watchpt_fault(void) {
    // use DFSR then DSCR
	//check DFSR first to see if event happened
	unsigned dfsr_val = cp15_dfsr_get();
	if (!bits_eq(dfsr_val, 0, 3, DEBUG_EVENT))
		return 0;
	unsigned dscr_val = cp14_dscr_get();
	if (!bits_eq(dscr_val, 2, 5, WATCHPOINT_EVENT))
		return 0;
	return 1;
}


//check bit 0 of wcr0 register to see if enabled
//pg. 13-22 of arm1176-ch13-debug.pdf
static inline int cp14_wcr0_is_enabled(void) {
	unsigned wcr0_val = cp14_wcr0_get();

	if (!bit_is_on(wcr0_val, 0))
		return 0;

	return 1;
}

// set bit 0 of wcr0 register to 1 to enable. 
// pg. 13-22 of arm1176-ch13-debug.pdf
static inline void cp14_wcr0_enable(void) {
	if (cp14_wcr0_is_enabled())
		panic("wcr0 already enabled.");

	//set enable bit, pg 13-22 of arm1176
	unsigned wcr0_val = 0;
	wcr0_val = bit_set(wcr0_val, 0);

	//set access bits, pg 13-22
	wcr0_val = bits_set(wcr0_val, 1, 2, 0b11);

	//set load/store access, pg. 13-22
	wcr0_val = bits_set(wcr0_val, 3, 4, 0b11);

	//set byte address select, pg. 13-22
	wcr0_val = bits_set(wcr0_val,  5, 8, 0b1111);

	//set watchpoint to match in secure and non-secure world (0b00)
	wcr0_val = bits_clr(wcr0_val, 14, 15);

	//disable linking
	wcr0_val = bit_clr(wcr0_val, 20);

	cp14_wcr0_set(wcr0_val);
}
static inline void cp14_wcr0_disable(void) {
	if (!cp14_wcr0_is_enabled())
		panic("Tried to disable wcr0 when wcr0 is already disabled.");
	
	unsigned wcr0_val = 0;
	cp14_wcr0_set(wcr0_val);
}

// Get watchpoint fault using WFAR
static inline uint32_t watchpt_fault_pc(void) {
	unsigned wfar_val = cp14_wfar_get();
	return wfar_val- 0x8;
}
    
#endif
