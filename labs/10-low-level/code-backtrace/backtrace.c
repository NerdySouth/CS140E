#include "backtrace.h"

int backtrace(frame_t f[], int max_frames) {
	void *fp = __builtin_frame_address (0);
	return 0;
}