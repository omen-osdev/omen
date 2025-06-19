#ifndef _DIRENT_H
#define _DIRENT_H

#include <omen/libraries/std/stdint.h>

struct dirent {
	/* Always zero */
	long d_ino;

	/* Position of next file in a directory stream */
	long d_off;

	/* Structure size */
	unsigned short d_reclen;

	/* File type */
	unsigned char d_type;

	/* File name */
	char d_name[1024];
} __attribute__((packed));

typedef struct dirent dirent;

struct DIR {
	struct dirent ent;
	long pos;
};
typedef struct DIR DIR;

#endif