// simple program to run a command when any file that is "interesting" in a directory
// changes.
// e.g., 
//      cmd-watch make
// will run make at each change.
//
// This should use the scandir similar to how you did `find_ttyusb`
//
// key part will be implementing two small helper functions (useful-examples/ will be 
// helpful):
//  - static int pid_clean_exit(int pid);
//  - static void run(char *argv[]);
//
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "libunix.h"

#define _SVID_SOURCE
#include <dirent.h>
#define SUCCESS 0


static int make_kid(int exit_code, char *argv[]) {
	int pid;
	if((pid = fork()) < 0)
		sys_die(fork, fork failed?);
	// child
	if(!pid) {
		debug("forked kid=%d\n", getpid());
		//prob execv here

		exit(exit_code);
	}
	return pid;
}

// scan the files in "./" (you can extend this) for those
// that match the suffixes in <suffixes> and check  if any
// have changed since the last time.
int check_activity(void) {
    char *suffixes[] = { ".c", ".h", ".S", "Makefile", 0 };
    const char *dirname = ".";
    int changed_p = 0;

    static time_t last_mtime;   // store last modification time.

    unimplemented();

    // return 1 if anything that matched <suffixes>
    return changed_p;
}

// synchronously wait for <pid> to exit.  returns 1 if it exited
// cleanly (via exit(0)), 0 otherwise.
static int pid_clean_exit(int pid) {
	int status;
	debug("testing <waitpid(-1, &status, 0)>\n");
	if(waitpid(pid, &status, 0) != pid)
		sys_die(waitpid, returned different pid);
	status = WEXITSTATUS(status);
	debug("\tkid=%d exited with %d\n\n", pid, status);
	if (status != SUCCESS) 
		return 0;
	return 1;
}

// simple helper to print null terminated vector of strings.
static void print_argv(char *argv[]) {
    assert(argv[0]);

	fprintf(stderr, "about to exec =<%s ", argv[0]);
	for(int i =1; argv[i]; i++)
		fprintf(stderr, " %s", argv[i]);
	fprintf(stderr, ">\n");
}


// fork/exec <argv> and wait for the result: print an error
// and exit if the kid crashed or exited with an error (a non-zero
// exit code).
static void run(char *argv[]) {
	int pid = make_kid(SUCCESS, argv);
	int status = pid_clean_exit(pid);
	if (status) {
		//success exit
		
		// result or something
	} else {
		//failed
		die("Child failed/crashed.\n");
	}
}

int main(int argc, char *argv[]) {
    if(argc < 2)
        die("cmd-watch: not enough arguments\n");

	//infinite loop, sleep for 250 ms each cycle
	//if check_activiy saystrue: 
		run(&argv[1]);
    return 0;
}
