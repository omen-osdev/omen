#ifndef _EXT2_BG_H
#define _EXT2_BG_H

#include <omen/libraries/std/stdint.h>

#include "ext2_structs.h"

int32_t ext2_operate_on_bg(struct ext2_partition * partition, uint8_t (*callback)(struct ext2_partition *, struct ext2_block_group_descriptor*, uint32_t));
uint8_t ext2_flush_bg(struct ext2_partition* partition, struct ext2_block_group_descriptor* bg, uint32_t bgid);
uint8_t ext2_dump_bg(struct ext2_partition* partition, struct ext2_block_group_descriptor * bg, uint32_t id);
uint8_t ext2_bg_has_free_inodes(struct ext2_partition * partition, struct ext2_block_group_descriptor * bg, uint32_t id);
void ext2_dump_all_bgs(struct ext2_partition* partition);
#endif /* _EXT2_BG_H */