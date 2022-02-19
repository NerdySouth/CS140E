#include "vm-ident.h"
#include "libc/bit-support.h"
#include "debug-fault.h"
volatile struct proc_state proc;

#define TRANSLATION_SECTION_FAULT  0b0101
#define PERMISSION_SECTION_FAULT 0b1101

// this is called by reboot: we turn off mmu so that things work.
void reboot_callout(void) {
    if(mmu_is_enabled())
        staff_mmu_disable();
}

void prefetch_abort_vector(unsigned lr) {
	// //cause of the exception
	// unsigned dfsr_val = cp15_dfsr_get();
	// // addr that the load or store was on.
    // unsigned addr = watchpt_fault_addr();
	// if (bits_eq(dfsr_val, 0, 3, 0b0101) && bit_is_off(dfsr_val, 10)) {
	// 	//was a tranlation section fault
	// 	//map the section
	// 	mmu_map_section(proc.pt, addr, addr, dom_id);
	// 	proc.fault_addr = *(unsigned *)addr;
	// 	proc.fault_count++;
	// }
}

void data_abort_vector(unsigned lr) {
	//cause of the exception
	unsigned dfsr_val = cp15_dfsr_get();
	// addr that the load or store was on.
    unsigned addr = cp15_far_get();

	//check type of fault: permission or translation
	if (bits_eq(dfsr_val, 0, 3, TRANSLATION_SECTION_FAULT) && bit_is_off(dfsr_val, 10)) {
		//was a tranlation section fault
		//map the section
		proc.fault_count++;
		//check addr is within 1MB of lowest SP
		if (addr <= proc.sp_lowest_addr + OneMB) {
			//grow stack
			mmu_map_section(proc.pt, (STACK_ADDR - (OneMB * (proc.fault_count + 1))), 
							(STACK_ADDR - (OneMB * (proc.fault_count + 1))), proc.dom_id);
		}
		staff_mmu_sync_pte_mods();
	} else if (bits_eq(dfsr_val, 0, 3, PERMISSION_SECTION_FAULT) && bit_is_off(dfsr_val, 10)) {
		// was a permission section fault
		if (proc.fault_addr == addr) {
			//map addr
			mmu_map_section(proc.pt, addr, addr, proc.dom_id);
		}	
	} 

}
