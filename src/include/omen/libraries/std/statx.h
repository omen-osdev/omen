#ifndef _STATX_H
#define _STATX_H

#include <omen/libraries/std/stdint.h>

struct statx_timestamp {
	uint64_t tv_sec;
	uint32_t tv_nsec;
	uint32_t __padding;
};

#define STATX_TYPE 0x1
#define STATX_MODE 0x2
#define STATX_NLINK 0x4
#define STATX_UID 0x8
#define STATX_GID 0x10
#define STATX_ATIME 0x20
#define STATX_MTIME 0x40
#define STATX_CTIME 0x80
#define STATX_INO 0x100
#define STATX_SIZE 0x200
#define STATX_BLOCKS 0x400
#define STATX_BASIC_STATS 0x7ff
#define STATX_BTIME 0x800
#define STATX_MNT_ID 0x1000
#define STATX_DIOALIGN 0x2000
#define STATX_ALL 0xfff

#define STATX_ATTR_COMPRESSED 0x4
#define STATX_ATTR_IMMUTABLE 0x10
#define STATX_ATTR_APPEND 0x20
#define STATX_ATTR_NODUMP 0x40
#define STATX_ATTR_ENCRYPTED 0x800
#define STATX_ATTR_AUTOMOUNT 0x1000
#define STATX_ATTR_MOUNT_ROOT 0x2000
#define STATX_ATTR_VERITY 0x100000
#define STATX_ATTR_DAX 0x200000

struct statx {
	uint32_t stx_mask;

	uint32_t stx_blksize;
	uint64_t stx_attributes;
	uint32_t stx_nlink;
	uint32_t stx_uid;
	uint32_t stx_gid;
	uint16_t stx_mode;
	uint16_t __padding;
	uint64_t stx_ino;
	uint64_t stx_size;
	uint64_t stx_blocks;
	uint64_t stx_attributes_mask;

	struct statx_timestamp stx_atime;
	struct statx_timestamp stx_btime;
	struct statx_timestamp stx_ctime;
	struct statx_timestamp stx_mtime;

	uint32_t stx_rdev_major;
	uint32_t stx_rdev_minor;
	uint32_t stx_dev_major;
	uint32_t stx_dev_minor;

	uint64_t stx_mnt_id;
	uint32_t stx_dio_mem_align;
	uint32_t stx_dio_offset_align;

	uint64_t __padding1[12];
};

#define AT_STATX_SYNC_TYPE      0x6000  /* Type of synchronisation required from statx() */
#define AT_STATX_SYNC_AS_STAT   0x0000  /* - Do whatever stat() does */
#define AT_STATX_FORCE_SYNC     0x2000  /* - Force the attributes to be sync'd with the server */
#define AT_STATX_DONT_SYNC      0x4000  /* - Don't sync attributes with the server */

#endif