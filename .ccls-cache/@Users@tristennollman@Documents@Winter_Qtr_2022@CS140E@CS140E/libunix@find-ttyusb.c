// engler, cs140e: your code to find the tty-usb device on your laptop.
#include <assert.h>
#include <fcntl.h>
#include <string.h>

#include "libunix.h"

#define _SVID_SOURCE
#include <dirent.h>
static const char *ttyusb_prefixes[] = {
	"ttyUSB",	// linux
	"cu.SLAB_USB", // mac os
	"cu.SLAB_USBtoUART",
    // if your system uses another name, add it.
	0
};

static int filter(const struct dirent *d) {
    // scan through the prefixes, returning 1 when you find a match.
    // 0 if there is no match.
	if (!strcmp(d->d_name, ttyusb_prefixes[0]) || !strcmp(d->d_name, ttyusb_prefixes[1]) ||
		!strcmp(d->d_name, ttyusb_prefixes[2])) {
		return 1;
	}
	return 0;
}

// find the TTY-usb device (if any) by using <scandir> to search for
// a device with a prefix given by <ttyusb_prefixes> in /dev
// returns:
//  - device name.
// error: panic's if 0 or more than 1 devices.
char *find_ttyusb(void) {
    // use <alphasort> in <scandir>
    // return a malloc'd name so doesn't corrupt.
	char *dirname = "/dev/\0";
	struct dirent **d_prefixes;
	//scan for ttys
	unsigned num_entries = scandir(dirname, &d_prefixes, filter, alphasort);

	//panic if we found less than or more than 1 match
	if (num_entries != 1) {
		panic("Num TTY USB Devices that match is %d.\n", num_entries);
	}

	//copy the tty name to a  buffer that will hold the abs path
	char *buf = malloc(sizeof(char) * (strlen(dirname) + (*d_prefixes)->d_namlen + 1));
	strcat(buf, dirname);
	strcat(buf, d_prefixes[0]->d_name);
	//return abs path of tty
	return buf;
}

// return the most recently mounted ttyusb (the one
// mounted last).  use the modification time 
// returned by state.
char *find_ttyusb_last(void) {
	return find_ttyusb();
}

// return the oldest mounted ttyusb (the one mounted
// "first") --- use the modification returned by
// stat()
char *find_ttyusb_first(void) {
	return find_ttyusb();
}
