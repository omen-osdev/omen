#pragma GCC diagnostic ignored "-Wvariadic-macros"

#include "ext2_bg.h"

#include "ext2_block.h"
#include "ext2_block.h"

#include "ext2_util.h"
#include "ext2_integrity.h"

#include <omen/libraries/std/stdint.h>
#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <disk/disk_interface.h>

int32_t ext2_operate_on_bg(struct ext2_partition * partition, int64_t (*callback)(struct ext2_partition *, struct ext2_block_group_descriptor*, uint32_t)) {
    uint32_t i;
    for (i = 0; i < partition->group_number; i++) {
        if (callback(partition, &partition->gd[i], i)) 
            return (int32_t)i;
    }

    return EXT2_BG_ERROR;
}

int64_t ext2_flush_bg(struct ext2_partition* partition, struct ext2_block_group_descriptor* bg, uint32_t bgid) {
    uint32_t block_size = 1024 << ((struct ext2_superblock*)(partition->sb))->s_log_block_size;

    uint32_t block_group_descriptors_size = DIVIDE_ROUNDED_UP(partition->group_number * sizeof(struct ext2_block_group_descriptor), partition->sector_size);
    uint32_t sectors_per_group = ((struct ext2_superblock*)(partition->sb))->s_blocks_per_group * (block_size / partition->sector_size);
    if (!write_disk(partition->disk, (uint8_t*)bg, partition->lba+(sectors_per_group*bgid)+partition->bgdt_block, block_group_descriptors_size)) {
        EXT2_ERROR("Failed to write block group descriptor table");
        return -1;
    }

    return 0;
}

void ext2_dump_all_bgs(struct ext2_partition* partition) {
    ext2_operate_on_bg(partition, ext2_dump_bg);
}

int64_t ext2_dump_bg(struct ext2_partition* partition, struct ext2_block_group_descriptor * bg, uint32_t id) {
    uint32_t block_size = 1024 << ((struct ext2_superblock*)(partition->sb))->s_log_block_size;

    DBG_DEBUG("Block group %d:\n", id);
    DBG_DEBUG("  Block bitmap: %d\n", bg->bg_block_bitmap);
    DBG_DEBUG("  Inode bitmap: %d\n", bg->bg_inode_bitmap);
    DBG_DEBUG("  Inode table: %d\n", bg->bg_inode_table);
    DBG_DEBUG("  Free blocks: %d\n", bg->bg_free_blocks_count);
    DBG_DEBUG("  Free inodes: %d\n", bg->bg_free_inodes_count);
    DBG_DEBUG("  Directories entries: %d\n", bg->bg_used_dirs_count);

    DBG_DEBUG("Dumping block bitmap:\n");
    uint8_t * block_bitmap = kmalloc(block_size);
    if (block_bitmap == 0) {
        EXT2_ERROR("Failed to allocate block bitmap");
        return 1;
    }

    if (ext2_read_block(partition, bg->bg_block_bitmap, block_bitmap) != 1) {
        EXT2_ERROR("Failed to read block bitmap");
        return 1;
    }
    
    for (uint32_t i = 0; i < block_size; i++) {
        DBG_DEBUG("%02x ", block_bitmap[i]);
    }
    DBG_DEBUG("\n");

    DBG_DEBUG("Dumping inode bitmap:\n");
    uint8_t * inode_bitmap = kmalloc(block_size);
    if (inode_bitmap == 0) {
        EXT2_ERROR("Failed to allocate inode bitmap");
        return 1;
    }

    if (ext2_read_block(partition, bg->bg_inode_bitmap, inode_bitmap) != 1) {
        EXT2_ERROR("Failed to read inode bitmap");
        return 1;
    }
    
    for (uint32_t i = 0; i < block_size; i++) {
        DBG_DEBUG("%02x ", inode_bitmap[i]);
    }
    DBG_DEBUG("\n");

    kfree(block_bitmap);
    kfree(inode_bitmap);
    return 0;
}

int64_t ext2_bg_has_free_inodes(struct ext2_partition * partition, struct ext2_block_group_descriptor * bg, uint32_t id) {
    (void)id;
    (void)partition;
    return bg->bg_free_inodes_count > 0;
}