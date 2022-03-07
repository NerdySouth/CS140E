#include "rpi.h"
#include "pi-sd.h"
#include "mbr.h"

#define MBR_LBA 0

mbr_t *mbr_read() {
	// Be sure to call pi_sd_init() before calling this function!

	// TODO: Read the MBR into a heap-allocated buffer.  Use `pi_sd_read` or
	// `pi_sec_read` to read 1 sector from LBA 0 into memory.
	void* data = pi_sec_read(MBR_LBA, 1);

	// TODO: Verify that the MBR is valid. (see mbr_check)
	mbr_check((struct mbr*) data);

	// TODO: Return the MBR.
	return (struct mbr*) data;
}
