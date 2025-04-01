//https://wiki.osdev.org/Ext2
#ifndef _EXT2_H
#define _EXT2_H

#include <omen/libraries/std/stdint.h>

#include "ext2_structs.h"

#define BGDT_BLOCK              1
#define BLOCK_NUMBER            4
#define MAX_DISK_NAME_LENGTH    32
#define EXT2_EOF                0xFFFFFFFF

#define EXT2_RESULT_ERROR 0
#define EXT2_RESULT_OK 1

#define EXT2_FILE_TYPE_UNKNOWN  0
#define EXT2_FILE_TYPE_REGULAR  1
#define EXT2_FILE_TYPE_DIRECTORY 2
#define EXT2_FILE_TYPE_CHARDEV  3
#define EXT2_FILE_TYPE_BLOCKDEV 4
#define EXT2_FILE_TYPE_FIFO     5
#define EXT2_FILE_TYPE_SOCKET   6
#define EXT2_FILE_TYPE_SYMLINK  7

struct ext2_partition * ext2_register_partition(const char* disk, uint32_t lba, const char* mountpoint);
uint8_t ext2_sync(struct ext2_partition * partition);
const char * ext2_get_partition_name(struct ext2_partition * partition);
uint32_t ext2_count_partitions();
struct ext2_partition * ext2_get_partition_by_index(uint32_t index);
uint8_t ext2_search(const char* name, uint32_t lba);
uint8_t ext2_unregister_partition(struct ext2_partition* partition);
void ext2_dump_partition(struct ext2_partition* partition);

uint8_t is_directory(struct ext2_directory_entry * dentry);
uint8_t is_regular_file(struct ext2_directory_entry * dentry);
uint8_t ext2_get_dentry(struct ext2_partition* partition, const char* path, struct ext2_directory_entry* dentry);
uint64_t ext2_get_file_size(struct ext2_partition* partition, const char* path);
uint8_t ext2_list_directory(struct ext2_partition* partition, const char * path);
uint8_t ext2_read_directory(struct ext2_partition* partition, const char * path, uint32_t * count, struct ext2_directory_entry** buffer);
uint8_t ext2_create_directory_entry(struct ext2_partition* partition, uint32_t inode_number, uint32_t child_inode, const char* name, uint32_t type);
uint8_t ext2_create_file(struct ext2_partition * partition, const char* path, uint32_t type, uint32_t permissions);
uint8_t ext2_resize_file(struct ext2_partition* partition, uint32_t inode_index, uint32_t new_size);
uint32_t ext2_get_inode_index(struct ext2_partition* partition, const char* path);
uint8_t ext2_read_file(struct ext2_partition * partition, const char * path, uint8_t * destination_buffer, uint64_t size, uint64_t skip);
uint8_t ext2_write_file(struct ext2_partition * partition, const char * path, uint8_t * source_buffer, uint64_t size, uint64_t skip);
uint8_t ext2_delete_file(struct ext2_partition* partition, const char * path);
//get stat
uint8_t ext2_get_stat(struct ext2_partition* partition, const char * path, void* st);
uint8_t ext2_set_debug_base(const char* base);
void ext2_inhibit_errors(uint8_t t);
uint16_t ext2_get_file_permissions(struct ext2_partition* partition, const char* path);
uint8_t ext2_debug(struct ext2_partition* partition);
uint8_t ext2_stacktrace();
uint8_t ext2_errors();
#endif