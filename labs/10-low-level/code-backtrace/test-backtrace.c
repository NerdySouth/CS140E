#include "rpi.h"

typedef struct {
	const char *name;
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



/*
 * `print_frames`
 *
 * Given an array of frames that has previously been filled in by a call to
 * backtrace(), `print_frames` prints the backtrace information, one line per
 * frame, using this format:
 *
 *     #0 0x85f8 at malloc+132
 *     #1 0x8868 at strndup+28
 *     #2 0x8784 at main+24
 *
 * @param f     array of stack frames
 * @param n     number of frames in array
 */
void print_frames(frame_t f[], int n);

/*
 * `print_backtrace`
 *
 * Convenience function that calls `backtrace` and `print_frames` to display
 * stack frames of currently executing program.
 */
void print_backtrace(void);




void a(void);
void notmain(void) {
    a();
}

static unsigned depth;
static void indent(unsigned n) {
    n *= 3;
    while(n-->0)
        output(" ");
}
static void push(const char *msg) { 
    indent(depth++);
    output("---> call %s\n", msg);
}
static void pop(const char *msg) { 
    indent(--depth);
    output("<--- ret %s\n", msg);
}


#define MK_STR(w) #w

#define gen_fn(x,y)                                             \
    void y (void);                                              \
    void x (void) { push(MK_STR(y)); y(); pop(MK_STR(y)); }

void f(void);

gen_fn(a,b)
gen_fn(b,c)
gen_fn(c,d)
gen_fn(d,f)

// void backtrace(void) {}
void f(void) { 
    indent(depth); 
    debug("done\n"); 
    backtrace();
}
