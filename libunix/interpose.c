#include <string.h>
#include "interpose-internal.h"

// fork/exec server the program in <argv>
// return pid
//
// maybe optional [not sure on macos]:
// - use <getrlimit> to get all open file descriptors and close them
//   in the child.
static int exec_server(char **argv) {
    trace_output("going to fork/exec: %s ", argv[0]);
    for(int i = 1; argv[i]; i++)
        output(" argv[%d]=<%s> ", i, argv[i]);
    output("\n");

    int pid;
	pid = fork();
	if (!pid) {
		//child process: my-install
		//close all  file  descriptors beyond 2
		
		execvp(argv[0], argv);
	} 
	//parent process: interpose
    return pid;
}

// call trace_err if the child died with a coredump
// otherwise return:
//   - -1 if the child hasn't exited.
//   - the exitcode.
static int get_exitcode_noblk(pid_t pid) {
	int status = 0;
	//wait without blocking
	int ret = waitpid(pid, &status, WNOHANG);
	if (WCOREDUMP(status)) { trace_err("process: %d core dumped.\n", pid); }
	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV) {
		//child died from  a coreduump (seg fault/SIGSEGV)
		//died from sigsevg, no core dump though
		trace_err("process: %d died from sigsev.\n", pid);
	}
	//if ret == 0 child has not exited, otherwise return exit status
	if (!ret) {
		return -1;
	}
	return WEXITSTATUS(status);
}

// called to check if the unix side exited cleanly with exit(0)
int exit_ok(pid_t pid) {
    int exitcode = get_exitcode_noblk(pid);
    if(exitcode < 0)
        return 0;
    if(exitcode == 0) {
        trace_ok("unix exited\n");
        return 1;
    }
    trace_err("unix crashed died with exitcode=%d\n", exitcode);
}

// use your <can_read> routine (write it) to check if
// file descriptor <from->fd> has data.
//
// if not: return 0.
// if so: 
//   1. read it 
//   2. forward (write()) it to <to->fd>
//   3.return the number of bytes forwarded.
//
// make sure you check all the error codes!
static int forward_u8(endpt_t *from, endpt_t *to) {
    if(!can_read(from->fd))
        return 0;

    int n;
    uint8_t b;
    if((n = read(from->fd, &b, 1)) < 0)
        panic("can't happen: %d\n", from->fd);

    // unix could have died, right?
    if(n == 0)
        panic("zero bytes from %s: are we done?\n", from->name);
    write_exact(to->fd, &b, 1);

    // construct the last uint32 we read from this stream: useful
    // for handling corruption.
    from->last_u32 = (from->last_u32 >> 8) | (b << 24);
    from->sent_nbytes ++;
    return 1;
}

// forward from the unix bootloader <u> to the pi side <p>
// give an error if takes longer than <timeout_sec>
//
// use waitpid to get the exitcode from the unix side: 
//   - must be 0.   
//   - give an error (using trace_err) if its non-zero.
//     if it's from a crash rather than an exit, indicate that too.
static int trace_ok_fn(endpt_t *u, endpt_t *p, unsigned timeout_sec) {
	int exit_code = get_exitcode_noblk(u->pid);
	//read the bytes from unix
	while (exit_code == -1) {
		//unix side has not exited

		//get time before forward to pi
		clock_t start_clock = clock();

		//forward from unix to pi
		forward_u8(u, p);

		//get clock after andd check time elapsed
		clock_t diff = clock() - start_clock;
		unsigned elapsed_sec = ((unsigned)diff) / CLOCKS_PER_SEC;

		if (elapsed_sec > timeout_sec) {
			return 0;
		}

		//mirror above from pi to unix
		start_clock = clock();

		//forward from pi to unix
		forward_u8(p, u);

		diff = clock() - start_clock;
		elapsed_sec = ((unsigned)diff) / CLOCKS_PER_SEC;

		if (elapsed_sec > timeout_sec) {
			return 0;
		}

		exit_code = get_exitcode_noblk(u->pid);
	}

	if (exit_ok(u->pid)) {
		return 1;
	}
	trace_err("Unix did not exit cleanly.\n");
	return 0;
}


// this should be a clean run of <install>: returns the number of
// bytes the unix side sent.
int clean_run(char *install, char *dev_name, char *pi_prog) {
    // open the pi side device.
    endpt_t p = start_pi_side(dev_name);
    endpt_t u = start_unix_side(install, pi_prog);

    assert(p.fd);
    assert(u.fd);

    if(!trace_ok_fn(&u, &p, 5))
        trace_err("ERROR: clean run failed?\n");
    int nbytes = u.sent_nbytes;

    endpt_destroy(&p);
    endpt_destroy(&u);

    return nbytes;
}

// forward from <u> <----> <p> until unix has sent <timetout_nbyte> of 
// data: at that point, do not send any more data to the pi: make
// sure the unix side exits with exit(1).
static int timeout_run_fn(endpt_t *u, endpt_t *p, unsigned timeout_nbyte) {
	//send timeout_nbytes of data to pi
	for (unsigned i = 0; i <= timeout_nbyte; i++) {
		forward_u8(u, p);
		forward_u8(p, u);
	}

	//make sure  unix side exits with  exit(1)
	if (get_exitcode_noblk(u->pid) != 1) {
		return 1;
	}
	return 0;
}

// timeout sending from unix to pi.
// should wrap this up in a subprocess.
int
timeout_run(char *install, char *dev_name, char *pi_prog, unsigned n) {
    // open the pi side device.
    endpt_t p = start_pi_side(dev_name);
    endpt_t u = start_unix_side(install, pi_prog);

    if(!timeout_run_fn(&u, &p, n))
        panic("pi shuld have failed\n");

    // if subprocess: don't have to do this.
    endpt_destroy(&p);
    endpt_destroy(&u);

    trace_output("done forwarding: pi exited with an error\n");
    return u.sent_nbytes;
}
