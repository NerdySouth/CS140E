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

static void thread_b_code(void *arg) {
	for (int i = 0; i < 100; i++) {
		printk("%d, ", i);
	}
	printk("\n");
	rpi_exit(0);
}

// trivial first thread: does not block, explicitly calls exit.
static void thread_a_code(void *arg) {
	//create the thread we wanna wait  on
	int *b_arg = kmalloc(sizeof *b_arg);
	rpi_thread_t *child_thread = rpi_fork(thread_b_code, b_arg);
	trace("about to wait on child thread...\n");

	rpi_wait(child_thread);
	trace("done.\n");

	trace("Exiting parent thread...\n");
	rpi_exit(0);
}



void notmain() {
    uart_init();
    kmalloc_init_set_start(1024 * 1024, 1024*1024);

    trace("about to test waiting on a thread (blocking)...\n");

	//setup waiter thread
	int *a_arg = kmalloc(sizeof *a_arg);
	rpi_fork(thread_a_code, a_arg);

	rpi_thread_start();

	// no more threads: check.
    printk("SUCCESS!\n");
	clean_reboot();
}
void print_and_die(void) { panic("should not call\n"); }

