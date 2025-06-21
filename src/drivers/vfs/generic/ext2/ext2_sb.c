#pragma GCC diagnostic ignored "-Wvariadic-macros"

#include "ext2_sb.h"
#include "ext2_integrity.h"

#include <omen/apps/debug/debug.h>
#include <disk/disk_interface.h>

int64_t ext2_flush_sb(struct ext2_partition* partition, struct ext2_block_group_descriptor* bg, uint32_t bgid) {
    (void)bg;
    if (bgid >= partition->backup_bgs_count || partition->backup_bgs[bgid] == -1) return -1;

    uint32_t block_size = 1024 << ((struct ext2_superblock*)(partition->sb))->s_log_block_size;
    uint32_t sectors_per_group = ((struct ext2_superblock*)(partition->sb))->s_blocks_per_group * (block_size / partition->sector_size);

    if (!write_disk(partition->disk, (uint8_t*)partition->sb, partition->lba+(sectors_per_group*bgid)+partition->sb_block, 2)) {
        return -1;
    }
    
    return 0;
}

void ext2_dump_sb(struct ext2_partition* partition) {
    struct ext2_superblock* sb = (struct ext2_superblock*)partition->sb;
    EXT2_INFO("ext2 superblock:\n");
    EXT2_INFO("s_inodes_count: %u\n", sb->s_inodes_count);
    EXT2_INFO("s_blocks_count: %u\n", sb->s_blocks_count);
    EXT2_INFO("s_r_blocks_count: %u\n", sb->s_r_blocks_count);
    EXT2_INFO("s_free_blocks_count: %u\n", sb->s_free_blocks_count);
    EXT2_INFO("s_free_inodes_count: %u\n", sb->s_free_inodes_count);
    EXT2_INFO("s_first_sb_block: %u\n", sb->s_first_sb_block);
    EXT2_INFO("s_log_block_size: %u\n", sb->s_log_block_size);
    EXT2_INFO("s_log_frag_size: %u\n", sb->s_log_frag_size);
    EXT2_INFO("s_blocks_per_group: %u\n", sb->s_blocks_per_group);
    EXT2_INFO("s_frags_per_group: %u\n", sb->s_frags_per_group);
    EXT2_INFO("s_inodes_per_group: %u\n", sb->s_inodes_per_group);
    EXT2_INFO("s_mtime: %u\n", sb->s_mtime);
    EXT2_INFO("s_wtime: %u\n", sb->s_wtime);
    EXT2_INFO("s_mnt_count: %u\n", sb->s_mnt_count);
    EXT2_INFO("s_max_mnt_count: %u\n", sb->s_max_mnt_count);
    EXT2_INFO("s_magic: %u\n", sb->s_magic);
    EXT2_INFO("s_state: %u\n", sb->s_state);
    EXT2_INFO("s_errors: %u\n", sb->s_errors);
    EXT2_INFO("s_minor_rev_level: %u\n", sb->s_minor_rev_level);
    EXT2_INFO("s_lastcheck: %u\n", sb->s_lastcheck);
    EXT2_INFO("s_checkinterval: %u\n", sb->s_checkinterval);
    EXT2_INFO("s_creator_os: %u\n", sb->s_creator_os);
    EXT2_INFO("s_rev_level: %u\n", sb->s_rev_level);
    EXT2_INFO("s_def_resuid: %u\n", sb->s_def_resuid);
    EXT2_INFO("s_def_resgid: %u\n", sb->s_def_resgid);
}
