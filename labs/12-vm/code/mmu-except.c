#include "vm-ident.h"
#include "libc/bit-support.h"
#include "debug-fault.h"
volatile struct proc_state proc;

#define TRANSLATION_SECTION_FAULT  0b0101
#define PERMISSION_SECTION_FAULT 0b1101
#define PRIVLEDGED_READ_WRITE 0b01 
#define USER_READ 0b10
#define FULL_ACCESS 0b11
#define SECTION_MASK 0xfff00000

// this is called by reboot: we turn off mmu so that things work.
void reboot_callout(void) {
    if(mmu_is_enabled())
        mmu_disable();
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

//helper to set permissions for an addr we faulted on one we know it was a read
//that caused it
void set_read_perms(unsigned addr) {
	if (in_kernel(addr)) {
		// in kernel, but want  special perms for first segment 
		// this is so we dont write over our kernel or null (can still read)
		if (addr < OneMB) {
			// should be read only for kernel, no access to user
			mmu_mark_sec_priv_readonly(proc.pt, addr, 1);
			return;
		}
		// in kernel, but want to have r/w access (device memory), give user none
		mmu_mark_sec_priv_rw(proc.pt, addr, 1);
		return;
	}
	//not kernel code, set user read only
	mmu_mark_sec_full_readonly(proc.pt, addr, 1);
}

//helper to set proper permissions for a given addr 
//given we know it was a write that  caused the fault
void set_write_perms(unsigned addr) {
	if (in_kernel(addr)) {
		// in kernel, but want  special perms for first segment 
		// this is so we dont write over our kernel or null (can still read)
		if (addr < OneMB) {
			// should be read only for kernel, no access to user
			panic("ERROR: Tried to write to kernel space Section 1. Dying.\n");
		}
		// in kernel, but want r/w access (device memory), give user none
		mmu_mark_sec_priv_rw(proc.pt, addr, 1);
		return;
	}
	//not kernel code, set user rw 
	mmu_mark_sec_full_rw(proc.pt, addr, 1);
}

// helper to set ke

//helper to check if the provided DSFR value is a translation fault
unsigned translation_section_fault(unsigned dfsr_val) {
	return bits_eq(dfsr_val, 0, 3, TRANSLATION_SECTION_FAULT) && bit_is_off(dfsr_val, 10);
}

//helper to check if provided DFSR val is a permission fault
unsigned permission_section_fault(unsigned dfsr_val) {
	return bits_eq(dfsr_val, 0, 3, PERMISSION_SECTION_FAULT) && bit_is_off(dfsr_val, 10);
}
void prefetch_abort_vector(unsigned lr) {
	//cause of the exception
	unsigned ifsr_val = cp15_ifsr_get();
	unsigned addr = cp15_ifar_get();
	if (translation_section_fault(ifsr_val)) {
		unsigned reason = 0b101;
		panic("ERROR: attempting to %s addr %x with translation error: dying [reason=%b]\n", "execute", addr, reason);
	} else if (permission_section_fault(ifsr_val))  {
		unsigned reason = 0b1101;
		panic("ERROR: attempting to %s addr %x with permission error: dying [reason=%b]\n", "execute", addr, reason);
	}	
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
		//check addr is within 1MB of stack
		if (addr >= proc.sp_lowest_addr - OneMB && addr < proc.sp_lowest_addr) {
			//grow stack
			unsigned new_addr = mmu_sec_bits_only(addr);
			mmu_map_section(proc.pt, new_addr, new_addr, proc.dom_id);
			staff_mmu_sync_pte_mods();	
			return;
		}
		char *attempt;
		unsigned reason  = 0b101;
		attempt = bit_is_off(dfsr_val, 11) ? "load" : "write";
		panic("ERROR: attempting to %s addr %x with translation eror: dying [reason=%b]\n", attempt, addr, reason);
	} else if (permission_section_fault(dfsr_val)) {
		// was a permission section fault
		if (proc.fault_addr == addr) {
			// determine needed perms
			if (bit_is_off(dfsr_val, 11)) {
				// attempted read
				// give proper permissions for where the adddress is (kernel, user)
				set_read_perms(addr);
				// mmu_mprotect(proc.pt, addr, 1, in_kernel(addr) ? PRIVLEDGED_READ_WRITE : USER_READ);
			} else {
				// attempted write
				set_write_perms(addr);
				// mmu_mprotect(proc.pt, addr, 1, in_kernel(addr) ? PRIVLEDGED_READ_WRITE : FULL_ACCESS);
			}
			proc.fault_count++;
			staff_mmu_sync_pte_mods();
			return;
		}	
		char *attempt;
		unsigned reason  = 0b1101;
		attempt = bit_is_off(dfsr_val, 11) ? "load" : "write";
		panic("ERROR: attempting to %s addr %x with permission error: dying [reason=%b]\n", attempt, addr, reason);
	} 

}
