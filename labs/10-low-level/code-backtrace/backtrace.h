#include "rpi.h"
typedef struct {
	const char *namel
	uintptr_t resume_addr;
	int resume_offset;
} frame_t;

//helpers for printing from test-backtrace
static unsigned depth;
static const char *name_of(uintptr_t fn_start_addr){
	uint32_t word = *(uint32_t *)(fn_start_addr - 4);
	if((word >> 24) == 0xff) {
		uint32_t length = word & ~0xff000000;
		return ((char *)(fn_start_addr)) - 4 - length;
	} else {
		return "???";
	}
}
void backtrace() {
    uintptr_t *cur_fp;
    __asm__("mov %0, fp" : "=r" (cur_fp));
    while(1) {
        frame_t frame;

        uintptr_t *caller_fp = (uintptr_t *)((char *)cur_fp - 12);
        uintptr_t *resume_addr = (uintptr_t *)((char *)cur_fp - 4);

        if (*caller_fp  == 0) break;

        frame.resume_addr = *resume_addr;
        frame.resume_offset = *resume_addr - ((*cur_fp) - 12);

        const char *name = name_of(GET32(*(cur_fp - 3)) - 12);
        frame.name = name;

        printk("Name: %s\n", frame.name);
        printk("Resume_addr: %p\n", frame.resume_addr);
        printk("Resume_offset: %d\n\n", frame.resume_offset);

        cur_fp = (uintptr_t *)*caller_fp;
    }
}
