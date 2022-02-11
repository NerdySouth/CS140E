/*
 * first basic test: will succeed even if you have not implemented context switching
 * as long as you:
 *  - simply have rpi_exit(0) and rpi_yield() return rather than context switch to 
 *    another thread.
 *  - [even more basic if you have issues: have rpi_fork simply run immediately]
 * 
 * this test is useful to make sure you have the rough idea of how the threads 
 * should work.  you should rerun when you have your full package working to 
 * ensure you get the same result.
 */

#include "rpi.h"
#include "rpi-thread.h"

static unsigned thread_count, thread_sum;


// trivial first thread: does not block, explicitly calls exit.
static void thread_a_code(void *arg) {
	//about to put main_t to sleep
	unsigned usec = 1000000;
	unsigned start = timer_get_usec();
	trace("putting main_t to sleep for %d microseconds.\n", usec);
	rpi_exact_sleep(usec);
	unsigned end = timer_get_usec();
	trace("START: %d, END: %d, DELAY: %d\n", start, end, usec);
}



void notmain() {
    uart_init();
    kmalloc_init_set_start(1024 * 1024, 1024*1024);

    trace("about to test sleeping on a thread...\n");

	//setup waiter thread
	int *a_arg = kmalloc(sizeof *a_arg);
	rpi_thread_t *main_t = rpi_fork(thread_a_code, a_arg);

	rpi_thread_start();

	
	// no more threads: check.
    printk("SUCCESS!\n");
	clean_reboot();
}
void print_and_die(void) { panic("should not call\n"); }

