#ifndef _MBR_H
#define _MBR_H
#include <omen/libraries/std/stdint.h>
#include "../vfs.h"

struct mbr_partition {
    uint8_t attributes;
    uint8_t chs_address_partition_start[3];
    uint8_t partition_type;
    uint8_t chs_address_partition_final[3];
    uint32_t lba_partition_start;
    uint32_t number_of_sectors;
} __attribute__((packed));

struct mbr_header {
    uint8_t boot_code[440];
    uint32_t disk_signature;
    uint16_t unused;
    struct mbr_partition partitions[4];
    uint16_t signature;
} __attribute__((packed));

uint32_t read_mbr(const char* disk, struct vfs_partition* partitions, void (*add_part)(struct vfs_partition*, uint32_t, uint32_t, uint8_t, uint8_t));
#endif