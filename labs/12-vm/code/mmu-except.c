#include "vm-ident.h"
#include "libc/bit-support.h"
#include "debug-fault.h"
volatile struct proc_state proc;

#define TRANSLATION_SECTION_FAULT  0b0101
#define PERMISSION_SECTION_FAULT 0b1101
#define PRIVLEDGED_READ_WRITE 0b01 
#define USER_READ 0b10
#define FULL_ACCESS 0b11

// this is called by reboot: we turn off mmu so that things work.
void reboot_callout(void) {
    if(mmu_is_enabled())
        staff_mmu_disable();
}

//helper to determine if addr is in the kernel space
uint32_t in_kernel(uint32_t addr) {
	if (addr <= INT_STACK_ADDR && addr >= INT_STACK_ADDR - OneMB) {
		//addr is within intterrupt stack
		return 1; 
	} else if (PAGE_TABLE_ADDR <= addr && addr <= PAGE_TABLE_ADDR + OneMB) {
		//addr is in page table section
		return 1;
	} else if (GPIO_START_ADDR <= addr && addr <= GPIO_END_ADDR + OneMB) {
		//addr is in gpio space
		return 1;
	} else if (addr <= OneMB) {
		//in kernel space for sure, our code starts at 0x8000
		return 1;
	}
	return 0;
}

//helper to check if the provided DSFR value is a translation fault
unsigned translation_section_fault(unsigned dfsr_val) {
	return bits_eq(dfsr_val, 0, 3, TRANSLATION_SECTION_FAULT) && bit_is_off(dfsr_val, 10);
}

//helper to check if provided DFSR val is a permission fault
unsigned permission_section_fault(unsigned dfsr_val) {
	return bits_eq(dfsr_val, 0, 3, PERMISSION_SECTION_FAULT) && bit_is_off(dfsr_val, 10);
}
void prefetch_abort_vector(unsigned lr) {
	unsigned addr = cp15_ifar_get();
	unsigned reason = 0b1101;
	panic("ERROR: attempting to %s addr %x: dying [reason=%b]\n", "execute", addr, reason);
}

void data_abort_vector(unsigned lr) {
	
	//cause of the exception
	unsigned dfsr_val = cp15_dfsr_get();
	// addr that the load or store was on.
    unsigned addr = cp15_far_get();

	//check type of fault: permission or translation
	if (translation_section_fault(dfsr_val)) {
		//was a tranlation section fault
		//map the section
		proc.fault_count++;
		//check addr is within 1MB of lowest SP
		if (addr <= proc.sp_lowest_addr + OneMB) {
			//grow stack
			mmu_map_section(proc.pt, (STACK_ADDR - (OneMB * (proc.fault_count + 1))), 
							(STACK_ADDR - (OneMB * (proc.fault_count + 1))), proc.dom_id);
			staff_mmu_sync_pte_mods();
			return;
		}
	} else if (permission_section_fault(dfsr_val)) {
		// was a permission section fault
		if (proc.fault_addr == addr) {
			// determine needed perms
			if (bit_is_off(dfsr_val, 11)) {
				// attempted read
				// give proper permissions for where the adddress is (kernel, user)
				mmu_mprotect(proc.pt, addr, 1, in_kernel(addr) ? PRIVLEDGED_READ_WRITE : USER_READ);
			} else {
				// attempted write
				mmu_mprotect(proc.pt, addr, 1, in_kernel(addr) ? PRIVLEDGED_READ_WRITE : FULL_ACCESS);
			}
			proc.fault_count++;
			staff_mmu_sync_pte_mods();
			return;
		}	
	} 

	//likely an unmapped address read/write
	char *attempt;
	unsigned reason;
	if (bit_is_off(dfsr_val, 11)) {
		attempt = "load";
		reason = 0b101;
	} else {
		attempt = "write";
		reason = 0b100;
	}
	panic("ERROR: attempting to %s addr %x: dying [reason=%b]\n", attempt, addr, reason);
}
