#include "rpi.h"
struct bt {
            struct bt *next;
            uint32_t sp;
            uint32_t lr;
            uint32_t pc;
};

//helpers for printing from test-backtrace
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

static const char *name_of(uintptr_t fn_start_addr){
	uint32_t word = *(uint32_t *)(fn_start_addr - 4);
	if((word >> 24) == 0xff) {
		uint32_t length = word & ~0xff000000;
		return ((char *)(fn_start_addr)) - 4 - length;
	} else {
		return "???";
	}
}
static void backtrace() {
	void *cur_fp = __builtin_frame_address (0);

  	int result = 0;
	struct bt *btrace = cur_fp - (sizeof(int *) * 3);
	const char* btrace_names[100];
	uint32_t pc_loc = (btrace->lr - btrace->pc + 12);
	while (btrace->next) {
		// if (result == 1)
		// 	pc_loc = (btrace->lr - btrace->pc + 12);
		uintptr_t *up_fp = (uintptr_t *)(btrace->next);
		if(btrace->next == NULL)
			break;


		uintptr_t up_pc = (uintptr_t)(btrace->pc);

		const char *name = name_of(up_pc - 12);
		btrace_names[result] = name;

		// output("name: %s\n", name);
		cur_fp = (void *)*(uint32_t *)(cur_fp - (sizeof(int *) * 3));
		result++;
		btrace  = cur_fp - (sizeof(int *) * 3);
	}

	//print out  call order
	for (int i = result - 1; i > 0; i--) {
		push(btrace_names[i]);
	}

	//print out where we were next
	indent(++depth);
	output("%s+%d\n", btrace_names[0], pc_loc);

	//print return order
	for (int i = 1; i < result; i++) {
		pop(btrace_names[i]);
	}
}
