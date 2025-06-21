#pragma GCC diagnostic ignored "-Wvariadic-macros"

#include "ext2_partition.h"

#include "ext2_integrity.h"
#include "ext2_util.h"

#include <omen/libraries/std/stdint.h>
#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <disk/disk_interface.h>

struct ext2_partition * ext2_partition_head = 0x0;

void ext2_partition_dump_partition(struct ext2_partition* partition) {
    EXT2_INFO("ext2 partition:\n");
    EXT2_INFO("name: %s\n", partition->name);
    EXT2_INFO("disk: %s\n", partition->disk);
    EXT2_INFO("lba: %u\n", partition->lba);
    EXT2_INFO("group_number: %u\n", partition->group_number);
    EXT2_INFO("sector_size: %u\n", partition->sector_size);
    EXT2_INFO("bgdt_block: %u\n", partition->bgdt_block);
    EXT2_INFO("sb_block: %u\n", partition->sb_block);
    EXT2_INFO("flush_required: %u\n", partition->flush_required);
    EXT2_INFO("sb: %p\n", (void*)partition->sb);
    EXT2_INFO("gd: %p\n", (void*)partition->gd);
    EXT2_INFO("next: %p\n", (void*)partition->next);
}

void ext2_disk_from_partition(char * destination, const char * partition) {
    uint32_t partition_name = strlen(partition);
    //iterate partition backwards to find the last 'p' character
    for (int i = partition_name - 1; i >= 0; i--) {
        if (partition[i] == 'p') {
            //copy the partition name to the destination
            memcpy(destination, partition, i);
            destination[i] = 0;
            return;
        }
    }
}

uint8_t ext2_check_status(const char* disk) {
    int status = get_disk_status(disk);
    EXT2_DEBUG("Checking disk %s status", disk);
    if (status != STATUS_READY) {
        if (init_disk(disk)) {
            EXT2_WARN("Failed to initialize disk");
            return 0;
        }
        if (get_disk_status(disk) != STATUS_READY) {
            EXT2_WARN("Disk not ready");
            return 0;
        }
    }
    return 1;
}

struct ext2_partition * get_partition(struct ext2_partition* partition, const char * partno) {
    while (partition != 0) {
        if (strcmp(partition->name, partno) == 0) {
            return partition;
        }
        partition = partition->next;
    }
    return 0;
}

struct ext2_partition * ext2_partition_get_partition_by_index(uint32_t index) {
    EXT2_INFO("Getting partition by index %d", index);
    struct ext2_partition * partition = ext2_partition_head;
    uint32_t partition_id = 0;

    while (partition != 0) {
        if (partition_id == index) {
            return partition;
        }
        partition = partition->next;
        partition_id++;
    }

    return 0;
}

uint32_t ext2_partition_count_partitions() {
    EXT2_INFO("Counting partitions");
    uint32_t count = 0;
    struct ext2_partition * partition = ext2_partition_head;

    while (partition != 0) {
        count++;
        partition = partition->next;
    }
    return count;
}

uint8_t ext2_partition_search(const char* name, uint32_t lba) {
    EXT2_INFO("Searching for ext2 partition %s:%d", name, lba);
    uint8_t bpb[1024];
    if (!read_disk(name, bpb, lba+2, 1024)) {
        EXT2_ERROR("Failed to read disk");
        return 0;
    }

    struct ext2_superblock *sb = (struct ext2_superblock *)bpb;
    if (sb->s_magic == EXT2_SUPER_MAGIC) {
        EXT2_INFO("Found ext2 partition %s:%d", name, lba);
        return 1;
    } else {
        EXT2_INFO("No ext2 partition found %s:%d", name, lba);
        return 0;
    }
}

uint8_t ext2_partition_unregister_partition(struct ext2_partition* partition) {
    EXT2_INFO("Unregistering partition %s\n", partition->name);
    (void)partition;
    EXT2_WARN("unregstr_ext2_partition Not implemented");
    return 0;
}

struct ext2_partition * ext2_partition_register_partition(const char* disk, uint32_t lba, const char* mountpoint) {
    EXT2_INFO("Registering partition on disk %s at LBA %d, mountpoint: %s", disk, lba, mountpoint);

    if (!ext2_check_status(disk)) {
        EXT2_WARN("Disk is not ready");
        return 0;
    }

    uint32_t sector_size;
    if (!ioctl_disk(disk, IOCTL_GET_SECTOR_SIZE, &sector_size)) {
        EXT2_ERROR("Failed to get sector size");
        return 0;
    }
    
    if (sector_size == 0 || sector_size > 4096) {
        EXT2_ERROR("Invalid sector size");
        return 0;
    }

    EXT2_DEBUG("Disk %s is ready, sector size is %d", disk, sector_size);

    uint8_t superblock_buffer[1024];
    if (!read_disk(disk, superblock_buffer, lba+SB_OFFSET_LBA, 1024)) {
        EXT2_ERROR("Failed to read superblock");
        return 0;
    }

    struct ext2_superblock * superblock = (struct ext2_superblock*)superblock_buffer;
    if (superblock->s_magic != EXT2_SUPER_MAGIC) {
        EXT2_ERROR("Invalid superblock magic");
        return 0;
    }

    uint32_t block_size = 1024 << superblock->s_log_block_size;
    uint32_t sectors_per_block = DIVIDE_ROUNDED_UP(block_size, sector_size);
    EXT2_DEBUG("First superblock found at LBA %d", lba+SB_OFFSET_LBA);
    EXT2_DEBUG("Superblock magic valid, ext2 version: %d", superblock->s_rev_level);
    EXT2_DEBUG("Block size is %d", block_size);
    EXT2_DEBUG("Sectors per block: %d", sectors_per_block);
    EXT2_DEBUG("Blocks count: %d", superblock->s_blocks_count);
    EXT2_DEBUG("Inodes count: %d", superblock->s_inodes_count);
    EXT2_DEBUG("Blocks per group: %d", superblock->s_blocks_per_group);
    EXT2_DEBUG("Inodes per group: %d", superblock->s_inodes_per_group);

    uint8_t bgdt_block = (block_size == 1024) ? 2 : 1;

    uint32_t block_groups_first  = DIVIDE_ROUNDED_UP(superblock->s_blocks_count, superblock->s_blocks_per_group);
    uint32_t block_groups_second = DIVIDE_ROUNDED_UP(superblock->s_inodes_count, superblock->s_inodes_per_group);

    if (block_groups_first != block_groups_second) {
        EXT2_ERROR("block_groups_first != block_groups_second");
        return 0;
    }

    EXT2_DEBUG("Block groups: %d", block_groups_first);
    EXT2_DEBUG("Checking if sb is valid in all block groups...");
    uint32_t blocks_per_group = superblock->s_blocks_per_group;
    uint32_t sectors_per_group = blocks_per_group * sectors_per_block;
    uint8_t dummy_sb_buffer[1024];
    int32_t backup_bgs[64] = {-1};
    uint32_t backup_bgs_count = 0;
    for (uint32_t i = 0; i < block_groups_first; i++) {
        if (!read_disk(disk, dummy_sb_buffer, lba+(i*sectors_per_group)+SB_OFFSET_LBA, 1024)) {
            EXT2_ERROR("Failed to read dummy superblock");
            return 0;
        }
        struct ext2_superblock * dummy_sb = (struct ext2_superblock*)dummy_sb_buffer;
        if (dummy_sb->s_magic == EXT2_SUPER_MAGIC) {
            backup_bgs[backup_bgs_count++] = i;
        }
    }
    EXT2_DEBUG("Found %d valid superblocks", backup_bgs_count);
    //TODO: Delete this sanity check
    uint32_t block_group_descriptors_size = DIVIDE_ROUNDED_UP(block_groups_first * sizeof(struct ext2_block_group_descriptor), sector_size);
    EXT2_DEBUG("Block group descriptors size: %d", block_group_descriptors_size);
    //TODO: End of sanity check

    void * block_group_descriptor_buffer = kmalloc(block_group_descriptors_size * sector_size);

    if (!read_disk(disk, (uint8_t*)block_group_descriptor_buffer, lba+(sectors_per_block*bgdt_block), block_group_descriptors_size*sector_size)) {
        EXT2_ERROR("Failed to read block group descriptor table");
        return 0;
    }

    struct ext2_block_group_descriptor * block_group_descriptor = (struct ext2_block_group_descriptor*)block_group_descriptor_buffer;

    EXT2_DEBUG("Registering partition %s:%d", disk, lba);

    struct ext2_partition * partition = ext2_partition_head;
    uint32_t partition_id = 0;

    if (partition == 0) {
        ext2_partition_head = kmalloc(sizeof(struct ext2_partition));
        partition = ext2_partition_head;
    } else {
        while (partition->next != 0) {
            partition = partition->next;
            partition_id++;
        }
        partition->next = kmalloc(sizeof(struct ext2_partition));
        partition = partition->next;
    }

    snprintf(partition->name, 32, "%s", mountpoint);
    snprintf(partition->disk, 32, "%s", disk);
    partition->group_number = block_groups_first;
    partition->lba = lba;
    partition->sector_size = sector_size;
    memcpy(partition->backup_bgs, backup_bgs, 64*sizeof(int32_t));
    partition->backup_bgs_count = backup_bgs_count;
    partition->sb = kmalloc(1024);
    partition->flush_required = 0;
    partition->sb_block = SB_OFFSET_LBA;
    partition->bgdt_block = bgdt_block; 
    partition->next = 0;
    memcpy(partition->sb, superblock, 1024);
    partition->gd = kmalloc(block_group_descriptors_size * sector_size);
    memcpy(partition->gd, block_group_descriptor, block_group_descriptors_size * sector_size);

    EXT2_DEBUG("Partition %s has: %d groups", partition->name, block_groups_first);

    return partition;
}