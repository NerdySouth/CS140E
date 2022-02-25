#include "breakpoint.h"
#include "debug-fault.h"
#include "armv6-debug-impl.h"
#include "reg-util.h"

void brkpt_mismatch_set(uint32_t addr)  {

	cp14_bvr0_set(addr);  // set the breakpoint on addr
	uint32_t bcr_val = 0;
    bcr_val = bits_set(bcr_val, 21, 22, 0b10); //set mismatch
    bcr_val = bit_clr(bcr_val, 20);	// no linking
    bcr_val = bits_clr(bcr_val, 14, 15);
    // b = bit_set(b, 5);
    bcr_val = bits_set(bcr_val, 5, 8, 0b1111);
    bcr_val = bits_set(bcr_val, 1, 2, 0b11);
    bcr_val = bit_set(bcr_val, 0);
	cp14_bcr0_set(bcr_val);
}

void brkpt_mismatch_stop(void) {
	printk("mode = %b\n", reg_get_mode());
	cp14_bcr0_disable();
}

void brkpt_mismatch_start(void)  {
	cp14_enable();
	cp14_bcr0_disable();

	cp14_bvr0_set(0);

	//bcr options on armv6 ch13
    uint32_t bcr_val = 0;
    bcr_val = bits_set(bcr_val, 21, 22, 0b10); //set mismatch
    bcr_val = bit_clr(bcr_val, 20);	// no linking
    bcr_val = bits_clr(bcr_val, 14, 15);
    // b = bit_set(b, 5);
    bcr_val = bits_set(bcr_val, 5, 8, 0b1111);
    bcr_val = bits_set(bcr_val, 1, 2, 0b11);
    bcr_val = bit_set(bcr_val, 0);
    cp14_bcr0_set(bcr_val);
}