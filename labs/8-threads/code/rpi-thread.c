// engler, cs140e: starter code for trivial threads package.
#include "rpi.h"
#include "rpi-thread.h"

// tracing code.
#undef trace
// set this to 0 to stop tracing
enum { trace_p = 1};
#define trace(args...) do { if(trace_p) output("THREAD: " args); } while(0)


/***********************************************************************
 * datastructures used by the thread code.
 *
 * you don't have to modify this.
 */

#define E rpi_thread_t
#include "libc/Q.h"

// currently only have a single run queue and a free queue.
// the run queue is FIFO.
static Q_t runq, freeq;
static rpi_thread_t *cur_thread;        // current running thread.
static rpi_thread_t *scheduler_thread;  // first scheduler thread.

// monotonically increasing thread id: won't wrap before reboot :)
static unsigned tid = 1;

/***********************************************************************
 * simplistic pool of thread blocks: used to make alloc/free faster (plus,
 * our kmalloc doesn't have free (other than reboot).
 *
 * you don't have to modify this.
 */

// total number of thread blocks we have allocated.
static unsigned nalloced = 0;

// keep a cache of freed thread blocks.  call kmalloc if run out.
static rpi_thread_t *th_alloc(void) {
    rpi_thread_t *t = Q_pop(&freeq);

    if(!t) {
        t = kmalloc_aligned(sizeof *t, 8);
        nalloced++;
    }
#   define is_aligned(_p,_n) (((unsigned)(_p))%(_n) == 0)
    demand(is_aligned(&t->stack[0],8), stack must be 8-byte aligned!);
    t->tid = tid++;
    return t;
}

static void th_free(rpi_thread_t *th) {
    // push on the front in case helps with caching.
    Q_push(&freeq, th);
}


/***********************************************************************
 * implement the code below.
 */

// stack offsets we expect.
enum {
    R4_OFFSET = 0,
    R5_OFFSET = 1,
    R6_OFFSET,
    R7_OFFSET,
    R8_OFFSET,
    R9_OFFSET,
    R10_OFFSET,
    R11_OFFSET,
    R14_OFFSET = 8,
    LR_OFFSET = 8
};

// return pointer to the current thread.  
rpi_thread_t *rpi_cur_thread(void) {
    return cur_thread;
}

// create a new thread.
rpi_thread_t *rpi_fork(void (*code)(void *arg), void *arg) {
    rpi_thread_t *thread = th_alloc();

    // write this so that it calls code,arg.
    void rpi_init_trampoline(void);
	thread->stack[THREAD_MAXSTACK - 10 + LR_OFFSET] = (unsigned)&rpi_init_trampoline;
	thread->fn = code;
	thread->arg = arg;
	thread->stack[THREAD_MAXSTACK - 10 + R4_OFFSET] = (unsigned)code;
	thread->stack[THREAD_MAXSTACK - 10 + R5_OFFSET] = (unsigned)arg;
	thread->waiters = NULL;
    /*
     * must do the "brain surgery" (k.thompson) to set up the stack
     * so that when we context switch into it, the code will be
     * able to call code(arg).
     *
     *  1. write the stack pointer with the right value.
     *  2. store arg and code into two of the saved registers.
     *  3. store the address of rpi_init_trampoline into the lr
     *     position so context switching will jump there.
     */
	
	thread->saved_sp = &thread->stack[THREAD_MAXSTACK - 10];

    trace("rpi_fork: tid=%d, code=%p, arg=%x, saved_sp=%p\n",
            thread->tid, code, arg, thread->saved_sp);

    Q_append(&runq, thread);
    return thread;
}


// exit current thread.
//   - if no more threads, switch to the scheduler.
//   - otherwise context switch to the new thread.
//     make sure to set cur_thread correctly!
void rpi_exit(int exitcode) {
	//first check if exiting thread had a waiter thread.
	if (cur_thread->waiters) {
		rpi_thread_t  *old_t = cur_thread;
		cur_thread = old_t->waiters;
		rpi_cswitch(&old_t->saved_sp, cur_thread->saved_sp);
	} else if (!Q_empty(&runq)) {
		rpi_thread_t *old_t = cur_thread;
		cur_thread = Q_pop(&runq);
		rpi_cswitch(&old_t->saved_sp, cur_thread->saved_sp);
	}
    // when you switch back to the scheduler thread:
    trace("rpi_exit: done running threads, back to scheduler\n");
	rpi_cswitch(&cur_thread->saved_sp, scheduler_thread->saved_sp);

}

// yield the current thread.
//   - if the runq is empty, return.
//   - otherwise: 
//      * add the current thread to the back 
//        of the runq (Q_append)
//      * context switch to the new thread.
//        make sure to set cur_thread correctly!
void rpi_yield(void) {
    rpi_thread_t *old = cur_thread;

    rpi_thread_t *t;
    if(!(t = Q_pop(&runq)))
        return;

    trace("rpi_yield: switching from %d %d\n", old->tid, t->tid);
	Q_append(&runq, cur_thread);
	cur_thread = t;
	rpi_cswitch(&old->saved_sp, t->saved_sp);
	
}

/*
 * starts the thread system.  
 * note: our caller is not a thread!  so you have to 
 * create a fake thread (assign it to scheduler_thread)
 * so that context switching works correctly.   your code
 * should work even if the runq is empty.
 */
void rpi_thread_start(void) {
	scheduler_thread = th_alloc();
	trace("scheduler thread=%p\n", scheduler_thread);

	// no other runnable thread: return.
	if(Q_empty(&runq)) {
		goto end;
	}

	//setup scheduler thread
	scheduler_thread->saved_sp = &(scheduler_thread->stack[THREAD_MAXSTACK - 10]);
	
	cur_thread = Q_pop(&runq);
	rpi_cswitch(&scheduler_thread->saved_sp, cur_thread->saved_sp);
	// th_free(cur_thread);

end:
    trace("done with all threads, returning\n");
}



// helper routine: can call from assembly with r0=sp and it
// will print the stack out.  it then exits.
// call this if you can't figure out what is going on in your
// assembly.
void rpi_print_regs(uint32_t *sp) {
    // use this to check that your offsets are correct.
    printk("cur-thread=%d\n", cur_thread->tid);
    printk("sp=%p\n", sp);

    // stack pointer better be between these.
    printk("stack=%p\n", &cur_thread->stack[THREAD_MAXSTACK]);
    assert(sp < &cur_thread->stack[THREAD_MAXSTACK]);
    assert(sp >= &cur_thread->stack[0]);
    for(unsigned i = 0; i < 9; i++) {
        unsigned r = i == 8 ? 14 : i + 4;
        printk("sp[%d]=r%d=%x\n", i, r, sp[i]);
    }
    clean_reboot();
}


// block caller until thread <th> completes.
//
// add the current thread to a list in the thread.
void rpi_wait(rpi_thread_t *th) {
	// add cur_thread to waiters list of <th>
	th->waiters = cur_thread;
	//set cur_thread to be thread we are about to context switch to
	cur_thread = th;
	//context switch to new cur_thread
	rpi_cswitch(&(th->waiters->saved_sp), cur_thread->saved_sp);
	// -> when rpi_exit() is called in new cur_thread, we will first check if it has any waiters
}

// put the current thread on a blocked queue: resume in 
// exactly // <usec> micro-seconds.  not before, not after.
// should give low-error resumption times.
void rpi_exact_sleep(uint32_t usec) {
	//just block caller thread with rpi_wait for a thread on a delay func
	//setup  delay thread
	int *delay_arg = kmalloc(sizeof *delay_arg);
	*delay_arg = usec;
	rpi_thread_t *delay_thread = rpi_fork((void *)delay_us, delay_arg);
}
