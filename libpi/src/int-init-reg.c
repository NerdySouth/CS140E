#include "rpi.h"
#include "rpi-interrupts.h"
#include "vector-base.h"

// initialize global interrupt state.
void int_init_reg(void* int_vector_reg) {
    // put interrupt flags in known state. 
    //  BCM2835 manual, section 7.5
    PUT32(Disable_IRQs_1, 0xffffffff);
    PUT32(Disable_IRQs_2, 0xffffffff);
    dev_barrier();

    /*
     * Copy in interrupt vector table and FIQ handler _table and _table_end
     * are symbols defined in the interrupt assembly file, at the beginning
     * and end of the table and its embedded constants.
     */
	printk("In int_init: %x\n", int_vector_reg);
	vector_base_set((void*)int_vector_reg);
}

#define UNHANDLED(msg,r) \
	panic("ERROR: unhandled exception <%s> at PC=%x\n", msg,r)
