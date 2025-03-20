#ifndef _EXT2_BLOCK_H
#define _EXT2_BLOCK_H

#include <omen/libraries/std/stdint.h>

#include "ext2_structs.h"

#define EXT2_READ_FAILED        0xFFFFFFFE
#define EXT2_WRITE_FAILED       0xFFFFFFFD
#define EXT2_DEALLOCATE_FAILED 0xFFFFFFFC

int64_t ext2_read_block(struct ext2_partition* partition, uint32_t block, uint8_t * destination_buffer);
int64_t ext2_write_block(struct ext2_partition* partition, uint32_t block, uint8_t * source_buffer);
int64_t ext2_read_inode_bytes(struct ext2_partition* partition, uint32_t inode_number, uint8_t * destination_buffer, uint64_t count, uint64_t skip);
int64_t ext2_write_inode_bytes(struct ext2_partition* partition, uint32_t inode_number, uint8_t * source_buffer, uint64_t count, uint64_t skip);
//Perhaps add allocate single block
uint8_t ext2_allocate_blocks(struct ext2_partition* partition, struct ext2_inode_descriptor_generic * inode, uint32_t blocks_to_allocate);
uint32_t ext2_deallocate_block(struct ext2_partition* partition, uint32_t block);
uint32_t ext2_deallocate_blocks(struct ext2_partition* partition, uint32_t *blocks, uint32_t block_number);
#endif /* _EXT2_BLOCK_H */