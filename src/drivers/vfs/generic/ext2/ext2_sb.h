#ifndef _EXT2_SB_H
#define _EXT2_SB_H

#include <stdint.h>

#include "ext2_structs.h"

//FS State
#define EXT2_FS_CLEAN       1
#define EXT2_FS_ERRORS      2

#define EXT2_DIRECT_BLOCKS 12
#define EXT2_INDIRECT_BLOCKS(blocksize, sizeo_of_entry) ((blocksize / sizeo_of_entry))
#define EXT2_DOUBLE_INDIRECT_BLOCKS(blocksize, sizeo_of_entry) ((blocksize / sizeo_of_entry) * (blocksize / sizeo_of_entry))
#define EXT2_TRIPLE_INDIRECT_BLOCKS(blocksize, sizeo_of_entry) ((blocksize / sizeo_of_entry) * (blocksize / sizeo_of_entry) * (blocksize / sizeo_of_entry))

//Error handling methods
#define EXT2_IGNORE_ERRORS  1
#define EXT2_REMOUNT_RO     2
#define EXT2_KERNEL_PANIC   3

//Creator OS id
#define EXT2_LINUX          0
#define EXT2_HURD           1
#define EXT2_MASIX          2
#define EXT2_FREEBSD        3
#define EXT2_LITES          4

//Optional feature flags
#define PREALLOCATE_BLOCKS_FLAG     0x0001
#define AFS_SERVER_INODES_FLAG      0x0002
#define HAS_JOURNAL_FLAG            0x0004
#define INODES_EXT_ATTRIBS_FLAG     0x0008
#define CAN_RESIZE_FS_FLAG          0x0010
#define HASH_DIRECTORY_INDEX_FLAG   0x0020

//Required feature flags
#define COMPRESSION_USED            0x0001
#define DIRECTORY_ENTRIES_TYPE_FILE 0x0002
#define FS_NEEDS_JOURNAL_REPLAY     0x0004
#define FS_USES_JOURNAL_DEVICE      0x0008

//Read-only feature flags
#define SPARSE_SUPERBLOCKS          0x0001
#define FS_USES_64BIT_FILESIZE      0x0002
#define DIR_ENTRIES_TYPE_BTREE      0x0004

uint8_t ext2_flush_sb(struct ext2_partition* partition, struct ext2_block_group_descriptor* bg, uint32_t bgid);
void ext2_dump_sb(struct ext2_partition* partition);
#endif /* _EXT2_SB_H */