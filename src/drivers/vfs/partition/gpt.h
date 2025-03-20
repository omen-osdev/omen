#ifndef _GPT_H
#define _GPT_H

#include <omen/libraries/std/stdint.h>
#include "../vfs.h"

#define GPT_SIGNATURE	0x5452415020494645
#define GPT_EFI_ENTRY   "c12a7328-f81f-11d2-ba4b-00a0c93ec93b"
#define GPT_NO_ENTRY    "00000000-0000-0000-0000-000000000000"

struct gpt_header {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t disk_guid[16];
    uint64_t partition_table_lba;
    uint32_t partition_count;
    uint32_t partition_entry_size;
    uint32_t partition_table_crc32;
    uint8_t reserved2[420];
} __attribute__((packed));

struct gpt_entry {
    uint8_t type_guid[16];
    uint8_t partition_guid[16];
    uint64_t first_lba;
    uint64_t last_lba;
    uint64_t flags;
    uint16_t name[36];
} __attribute__((packed));

uint32_t read_gpt(const char* disk, struct vfs_partition* partitions, void (*add_part)(struct vfs_partition*, uint32_t, uint32_t, uint8_t, uint8_t));
//uint8_t test_disk(const char*, struct partition*);
#endif