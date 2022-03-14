/*
 * put platform-specific routines for:
 *  - esp_read
 *  - exp_write_exact
 *  - esp_has_data_timeout.
 */
#include "libesp.h"

static int log_fd = 0;
void esp_log(int fd) {
    log_fd = fd;
}

// called when the lex buffer has no characters.  reads in more from the esp.
int esp_read(lex_t *l, void *buf, unsigned maxbytes, unsigned usec) {
	// int r;

    // if(!esp_has_data_timeout(l, usec))
    //     return 0;
    // if((r = read_exact(l->h, buf, maxbytes)) <= 0)
    //     panic(read, failed);

    // // write out what we are reading.
    // if(log_fd) 
    //     write_exact(log_fd, buf, r);

    // return r;
	unimplemented();
}

// returns <n> if wrote, 0 otherwise.
int esp_write_exact(lex_t *l, const void *buf, unsigned n) {
    unimplemented();
    return n;
}

int esp_has_data_timeout(lex_t *l, unsigned usec) {
    unimplemented();
    return 0;
}

// sleep for <usec> microseconds.
int esp_usleep(unsigned usec) {
    delay_us(usec);
    return usec;
}
