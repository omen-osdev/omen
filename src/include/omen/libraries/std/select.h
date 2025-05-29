#ifndef _SELECT_H
#define _SELECT_H

#include <omen/libraries/std/stdint.h>
typedef struct {
	union {
		uint8_t elems[128];
		// Some programs require the fds_bits field to be present
		uint8_t fds_bits[128];
	};
} fd_set;
#endif