#ifndef _EXT2_PARTITION_H
#define _EXT2_PARTITION_H

#include <omen/libraries/std/stdint.h>
#include "ext2_structs.h"

#define SB_OFFSET_LBA           2
#define EXT2_SUPER_MAGIC        0xEF53

void ext2_disk_from_partition(char * destination, const char * partition);
uint8_t ext2_check_status(const char* disk);
struct ext2_partition * get_partition(struct ext2_partition* partition, const char * partno);
void ext2_partition_dump_partition(struct ext2_partition* partition);
struct ext2_partition * ext2_partition_register_partition(const char* disk, uint32_t lba, const char* mountpoint);
struct ext2_partition * ext2_partition_get_partition_by_index(uint32_t index);
uint32_t ext2_partition_count_partitions();
uint8_t ext2_partition_search(const char* name, uint32_t lba);
uint8_t ext2_partition_unregister_partition(struct ext2_partition* partition);
#endif /* _EXT2_PARTITION_H */