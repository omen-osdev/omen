#ifndef _EXT2_DENTRY_H
#define _EXT2_DENTRY_H

#include <omen/libraries/std/stdint.h>

#include "ext2_structs.h"

#define EXT2_NAME_LEN           255

//Directory entry types
#define EXT2_DIR_TYPE_UNKNOWN   0
#define EXT2_DIR_TYPE_REGULAR   1
#define EXT2_DIR_TYPE_DIRECTORY 2
#define EXT2_DIR_TYPE_CHARDEV   3
#define EXT2_DIR_TYPE_BLOCKDEV  4
#define EXT2_DIR_TYPE_FIFO      5
#define EXT2_DIR_TYPE_SOCKET    6
#define EXT2_DIR_TYPE_SYMLINK   7

int64_t ext2_create_directory_entry(struct ext2_partition* partition, uint32_t inode_number, uint32_t child_inode, const char* name, uint32_t type);
uint8_t ext2_delete_dentry(struct ext2_partition* partition, const char * path);
void ext2_list_dentry(struct ext2_partition* partition, const char * path);
uint8_t ext2_initialize_directory(struct ext2_partition* partition, uint32_t inode_number, uint32_t parent_inode_number);
uint8_t ext2_dentry_get_dentry(struct ext2_partition* partition, const char* parent_path, const char* name, struct ext2_directory_entry* entry);
uint32_t ext2_get_all_dirs(struct ext2_partition* partition, const char* parent_path, struct ext2_directory_entry** entries);
uint8_t ext2_operate_on_dentry(struct ext2_partition* partition, const char* path, uint8_t (*callback)(struct ext2_partition* partition, uint32_t inode_entry));
#endif /* _EXT2_DENTRY_H */