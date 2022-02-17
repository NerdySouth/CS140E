#include "backtrace.h"
#include "rpi.h"




const char *name_of(uintptr_t fn_start_addr) {
  uint32_t word = *(uint32_t *)(fn_start_addr - 4);
  if((word >> 24) == 0xff) {
    uint32_t length = word & ~0xff000000;
    return ((char *)(fn_start_addr)) - 4 - length;
  } else {
    return "???";
  }
}

void backtrace() {
	// void *fp = __builtin_frame_address (0);

	// struct bt *trace = fp - (sizeof(int *) * 3);
	// while (trace->next) {
	// 	//saved fp = fp  +  4 32bit words -> address of previous  functions pc
	// 	trace("saved fp=%p\n", trace->next);
	// 	//resume addr -> saved lr
	// 	trace("saved lr=%p\n", trace->lr);
	// 	trace("sp=%p\n", trace->sp);
	// 	trace("fp=%p\n\n", fp);

	// 	//update fp
	// 	trace = trace->next - (sizeof(int *) * 3);
	// }


	// return 0;

	void *cur_fp = __builtin_frame_address (0);

  	int result = 0;
	struct bt *btrace  = cur_fp - (sizeof(int *) * 3);
	const char* btrace_names[100];
	const uint32_t pc_loc = btrace->lr - btrace->pc + 12;
	while (btrace->next) {
		btrace  = cur_fp - (sizeof(int *) * 3);
		uintptr_t *up_fp = (uintptr_t *)(btrace->next);
		if(btrace->next == NULL)
			break;


		uintptr_t up_pc = (uintptr_t)(btrace->pc);

		const char *name = name_of(up_pc - 12);
		btrace_names[result] = name;

		// output("name: %s\n", name);
		cur_fp = (void *)*(uint32_t *)(cur_fp - (sizeof(int *) * 3));
		result++;
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

  return result;
}