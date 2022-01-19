#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "libunix.h"
// read in file <name>
// returns:
//  - pointer to the code.  pad code with 0s up to the
//    next multiple of 4.  
//  - bytes of code in <size>
//
// fatal error open/read of <name> fails.
// 
// How: 
//    - use stat to get the size of the file.
//    - round up to a multiple of 4.
//    - allocate a buffer  --- 
//    - zero pads to a // multiple of 4.
//    - read entire file into buffer.  
//    - make sure any padding bytes have zeros.
//    - return it.   
//
// make sure to close the file descriptor (this will
// matter for later labs).
void *read_file(unsigned *size, const char *name) {
	struct stat file_stat;
	int fd;
	//get file descriptor
	fd = open(name, O_RDONLY);
	
	//get file size
	if (!fstat(fd, &file_stat)) {
		unsigned file_size = file_stat.st_size;
		*size = file_size;
		//pad file to multiple of 4
		if (file_size % 4 != 0) {
			file_size += (file_size % 4);
		}
		//allocate zeroed memory
		void *buf = calloc(1, file_size + 4);

		unsigned bytes_read = read(fd, buf, file_size);
		return buf;
	}
	close(fd);
	void *empty_buf = NULL;
	return empty_buf;
}
