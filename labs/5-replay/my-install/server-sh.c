// engler, cs140e: driver for "bootloader" for an r/pi connected via 
// a tty-USB device.
//
// To make grading easier:
//              DO NOT MODIFY THIS CODE!
//              DO NOT MODIFY THIS CODE!
//              DO NOT MODIFY THIS CODE!
//              DO NOT MODIFY THIS CODE!
//              DO NOT MODIFY THIS CODE!
//
// You shouldn't have to modify any code in this file.  Though, if you find
// a bug or improve it, let me know!
//
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <termios.h>
#include <stdio.h>

#include "libunix.h"
#include "simple-boot.h"

// for simplicity support exactly three invocations:
//      my-install <program.name>
//      my-install /dev/<ttyusb name> <program.name>
//      my-install /dev/<ttyusb name> <program.name>  -exec [...]
#include <ctype.h>
int main(int argc, char *argv[]) { 
    char *progname = argv[0];

    // we don't use the -exec option til lab 11.  we just put it in now to save
    // time later.
    char **exec_argv = 0;
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-exec") == 0) {
            argv[i++] = 0;
            if(i == argc)
                die("%s: bad use of -exec: need a program name\n", progname);
            argc = i-1;
            exec_argv = &argv[i];
            output("exec: <%s>\n", exec_argv[0]);
        }
    }

    // 1. get the name of the ttyUSB.
    char *dev_name;
    if(argc == 2)
        dev_name = find_ttyusb_last();
    else if(argc == 3)
        dev_name = argv[1];
    else
        die("%s: wrong number (n=%d) of arguments: expected 1 or 2\n", progname, argc-1);

    // 2. open the ttyUSB in 115200, 8n1 mode
    int fd = set_tty_to_8n1(open_tty(dev_name), B115200, 1);

	// prompt user
	char *command = malloc(sizeof(char) * 21);
	assert(command);
	while (1)  {
		printf("Enter a command: ");
		scanf("%s", command);
		char *EXIT = "exit\0";
		if (strncmp(command, EXIT, 5)) {
			break;
		} else {
			put_uint32(fd, 0x42069);
		}
	}
	close(fd);
	return 0;
}
