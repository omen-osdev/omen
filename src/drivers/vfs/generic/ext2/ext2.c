//Supress -Waddress-of-packed-member warning
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
//Supress  warning: ISO C99 requires at least one argument for the "..." in a variadic macro
#pragma GCC diagnostic ignored "-Wvariadic-macros"

#include "ext2.h"

#include "ext2_bg.h"
#include "ext2_block.h"
#include "ext2_dentry.h"
#include "ext2_inode.h"
#include "ext2_partition.h"
#include "ext2_sb.h"

#include "ext2_util.h"
#include "ext2_integrity.h"

#include <omen/libraries/std/stdint.h>
#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <disk/disk_interface.h>

uint8_t flushing_required = 0;

#define EXT2_DENTRY_TRANSLATOR_INDEX 0
#define EXT2_INODE_TRANSLATOR_INDEX 1
const uint32_t file_type_translator[8][2] = {
    {EXT2_DIR_TYPE_UNKNOWN, 0},
    {EXT2_DIR_TYPE_REGULAR, INODE_TYPE_FILE},
    {EXT2_DIR_TYPE_DIRECTORY, INODE_TYPE_DIR},
    {EXT2_DIR_TYPE_CHARDEV, INODE_TYPE_CHARDEV},
    {EXT2_DIR_TYPE_BLOCKDEV, INODE_TYPE_BLOCKDEV},
    {EXT2_DIR_TYPE_FIFO, INODE_TYPE_FIFO},
    {EXT2_DIR_TYPE_SOCKET, INODE_TYPE_SOCKET},
    {EXT2_DIR_TYPE_SYMLINK, INODE_TYPE_SYMLINK}
};
char * ext2_file_type_names[8] = {
    "unknown",
    "regular",
    "directory",
    "chardev",
    "blockdev",
    "fifo",
    "socket",
    "symlink"
};

#define EXT2_TRANSLATE_UNIT(native, unit) ((file_type_translator[native][unit]))
#define EXT2_TRANSLATE_DENTRY_TO_NATIVE(dentry) ({\
    uint32_t i = 0;\
    for (; i < 8; i++) {\
        if (file_type_translator[i][EXT2_DENTRY_TRANSLATOR_INDEX] == dentry) {\
            break;\
        }\
    }\
    i;\
})
#define EXT2_TRANSLATE_INODE_TO_NATIVE(inode) ({\
    uint32_t i = 0;\
    for (; i < 8; i++) {\
        if (file_type_translator[i][EXT2_INODE_TRANSLATOR_INDEX] == inode) {\
            break;\
        }\
    }\
    i;\
})
#define EXT2_TRANSLATE_NATIVE_TO_DENTRY(native) (EXT2_TRANSLATE_UNIT(native, EXT2_DENTRY_TRANSLATOR_INDEX))
#define EXT2_TRANSLATE_NATIVE_TO_INODE(native) (EXT2_TRANSLATE_UNIT(native, EXT2_INODE_TRANSLATOR_INDEX))

uint8_t is_directory(struct ext2_directory_entry * dentry) {
    return (dentry->file_type == EXT2_DIR_TYPE_DIRECTORY);
}

uint8_t is_regular_file(struct ext2_directory_entry * dentry) {
    return (dentry->file_type == EXT2_DIR_TYPE_REGULAR);
}

//Returns EXT2_RESULT_ERROR on error, struct ext2_partition * on success
uint8_t ext2_sync(struct ext2_partition * partition) {
    EXT2_INFO("Syncing partition %s", partition->name);
    ext2_flush_partition(partition);
    return EXT2_RESULT_OK;
}

//Returns number of partitions, cannot fail
uint32_t ext2_count_partitions() {
    return ext2_partition_count_partitions();
}

struct ext2_partition * ext2_register_partition(const char* disk, uint32_t lba, const char* mountpoint) {
    return ext2_partition_register_partition(disk, lba, mountpoint);
}

//Returns struct ext2_partition *, on error returns EXT2_RESULT_ERROR
struct ext2_partition * ext2_get_partition_by_index(uint32_t index) {
    return ext2_partition_get_partition_by_index(index);
}

uint8_t ext2_search(const char* name, uint32_t lba) {
    return ext2_partition_search(name, lba);
}

uint8_t ext2_unregister_partition(struct ext2_partition* partition) {
    return ext2_partition_unregister_partition(partition);
}

uint8_t split_into_path_and_name(const char* full_path, char* parent, char* name) {
    // Check if the full path is empty
    if (strlen(full_path) == 0) {
        return 0;
    }

    // Find the last occurrence of the directory separator
    const char* last_sep = strrchr(full_path, '/');
    if (last_sep == NULL) {
        // No separator found, so assume the full path is a file name
        strcpy(parent, "");
        strcpy(name, full_path);
    } else {
        // Split the full path into parent directory and file name
        size_t parent_len = last_sep - full_path;
        if (parent_len == 0) {
            parent_len = 1;  // special case for root directory
        }
        strncpy(parent, full_path, parent_len);
        parent[parent_len] = '\0';
        strcpy(name, last_sep + 1);
    }
    return 1;
}

//Returns uint64, on error returns EXT2_RESULT_ERROR
uint64_t ext2_get_file_size(struct ext2_partition* partition, const char* path) {
    EXT2_INFO("Getting file size of %s", path);
    uint32_t inode_number = ext2_path_to_inode(partition, path);
    if (inode_number == EXT2_INO_PTI_ERROR) {
        return EXT2_RESULT_ERROR;
    }

    struct ext2_inode_descriptor_generic * inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_number);
    if (inode == EXT2_INO_PTI_ERROR) {
        return EXT2_RESULT_ERROR;
    }

    return inode->i_size;
}

//Returns partition name, cannot fail
const char * ext2_get_partition_name(struct ext2_partition * partition) {
    EXT2_INFO("Getting partition name");
    return partition->name;
}

uint8_t ext2_set_debug_base(const char* base) {
    EXT2_INFO("Setting debug base path to %s", base);
    ext2_set_debug_base_path(base);
    return EXT2_RESULT_OK;
}

uint8_t ext2_list_directory(struct ext2_partition* partition, const char * path) {
    EXT2_INFO("Listing directory %s", path);
    ext2_list_dentry(partition, path);
    return EXT2_RESULT_OK;
}

uint8_t ext2_get_dentry(struct ext2_partition* partition, const char* path, struct ext2_directory_entry* dentry) {
    EXT2_INFO("Getting dentry of %s", path);
    if (path == 0) {
        return EXT2_RESULT_ERROR;
    }

    char * parent_path = kmalloc(strlen(path) + 1);
    char * name = kmalloc(strlen(path) + 1);
    if (split_into_path_and_name(path, parent_path, name) == 0) {
        kfree(parent_path);
        kfree(name);
        return EXT2_RESULT_ERROR;
    }

    if (ext2_dentry_get_dentry(partition, parent_path, name, dentry)) {
        kfree(parent_path);
        kfree(name);
        return EXT2_RESULT_ERROR;
    }

    kfree(parent_path);
    kfree(name);
    return EXT2_RESULT_OK;
}

uint8_t ext2_read_directory(struct ext2_partition* partition, const char * path, uint32_t * count, struct ext2_directory_entry** buffer) {
    EXT2_INFO("Reading directory %s", path);
    *count = ext2_get_all_dirs(partition, path, buffer);
    return EXT2_RESULT_OK;
}

uint8_t ext2_create_file(struct ext2_partition * partition, const char* path, uint32_t type, uint32_t permissions) {
    EXT2_INFO("Creating file %s of type %s", path, ext2_file_type_names[type]);
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);

    char * name;
    char * parent_path;
    if (!ext2_path_to_parent_and_name(path, &parent_path, &name)) {
        EXT2_WARN("Invalid path");
        return EXT2_RESULT_ERROR;
    }

    uint32_t parent_inode_index = ext2_path_to_inode(partition, parent_path);
    if (parent_inode_index == EXT2_INO_PTI_ERROR) {
        EXT2_WARN("Parent directory doesn't exist");
        return EXT2_RESULT_ERROR;
    }

    struct ext2_inode_descriptor_generic * parent_inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, parent_inode_index);
    if (!(parent_inode->i_mode & INODE_TYPE_DIR)) {
        EXT2_WARN("Parent inode is not a directory");
        return EXT2_RESULT_ERROR;
    }

    EXT2_DEBUG("Parent inode is a directory, ready to allocate");

    uint32_t new_inode_index = ext2_allocate_inode(partition);
    if (!new_inode_index) {
        EXT2_ERROR("Failed to allocate inode");
        return EXT2_RESULT_ERROR;
    }

    EXT2_DEBUG("Allocated inode %d", new_inode_index);
    struct ext2_inode_descriptor * inode = ext2_initialize_inode(partition, new_inode_index, EXT2_TRANSLATE_NATIVE_TO_INODE(type), permissions);
    EXT2_DEBUG("Initialized inode %d", new_inode_index);

    if (ext2_write_inode(partition, new_inode_index, inode)) {
        EXT2_ERROR("Failed to write inode");
        return EXT2_RESULT_ERROR;
    }

    if (type == EXT2_FILE_TYPE_DIRECTORY) {
        EXT2_DEBUG("Creating directory");
        if (ext2_resize_file(partition, new_inode_index, block_size) != EXT2_RESULT_OK) {
            EXT2_ERROR("Failed to resize file");
            return EXT2_RESULT_ERROR;
        }

        ext2_flush_partition(partition);

        if (ext2_initialize_directory(partition, new_inode_index, parent_inode_index)) {
            EXT2_ERROR("Failed to initialize directory");
            return EXT2_RESULT_ERROR;
        }
    }

    EXT2_DEBUG("Creating directory entry for inode %d", new_inode_index);
    if (ext2_create_directory_entry(partition, parent_inode_index, new_inode_index, name, EXT2_TRANSLATE_NATIVE_TO_DENTRY(type))) {
        EXT2_ERROR("Failed to create directory entry");
        return EXT2_RESULT_ERROR;
    }
    ext2_list_directory(partition, path);

    return EXT2_RESULT_OK;
}

uint8_t ext2_resize_file(struct ext2_partition* partition, uint32_t inode_index, uint32_t new_size) {
    EXT2_INFO("Resizing file %d to %d", inode_index, new_size);
    struct ext2_inode_descriptor_generic * inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_index);
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    if (!inode) {
        EXT2_ERROR("Failed to read inode");
        return EXT2_RESULT_ERROR;
    }

    if (inode->i_size == new_size) {
        EXT2_WARN("File is already of the requested size");
        return EXT2_RESULT_OK;
    }

    if (inode->i_size > new_size) {
        uint32_t blocks_to_deallocate = (inode->i_size - new_size) / block_size;

        EXT2_DEBUG("Resizing file to %d bytes, deallocating %d blocks", new_size, blocks_to_deallocate);
        uint32_t * blocks = ext2_load_block_list(partition, inode_index);
        if (!blocks) {
            EXT2_ERROR("Failed to load block list");
            return EXT2_RESULT_ERROR;
        }

        if (ext2_deallocate_blocks(partition, blocks, blocks_to_deallocate) != blocks_to_deallocate) {
            EXT2_ERROR("Failed to deallocate blocks");
            return EXT2_RESULT_ERROR;
        }
    } else {
        uint32_t blocks_to_allocate = (new_size - inode->i_size) / block_size;
        if ((new_size - inode->i_size) % block_size) blocks_to_allocate++;

        EXT2_DEBUG("Resizing file to %d bytes, allocating %d blocks", new_size, blocks_to_allocate);
        if (ext2_allocate_blocks(partition, inode, blocks_to_allocate)) {
            EXT2_ERROR("Failed to allocate blocks");
            return EXT2_RESULT_ERROR;
        }   
    }

    inode->i_size = new_size;
    inode->i_sectors = DIVIDE_ROUNDED_UP(new_size, partition->sector_size);

    if (ext2_write_inode(partition, inode_index, (struct ext2_inode_descriptor*)inode)) {
        EXT2_ERROR("Failed to write inode");
        return EXT2_RESULT_ERROR;
    }

    ext2_flush_partition(partition);
    return EXT2_RESULT_OK;
}

uint8_t ext2_read_file(struct ext2_partition * partition, const char * path, uint8_t * destination_buffer, uint64_t size, uint64_t skip) {
    EXT2_INFO("Reading file %s", path);

    uint32_t inode_index = ext2_path_to_inode(partition, path);

    if (inode_index == EXT2_INO_PTI_ERROR) {
        EXT2_WARN("Failed to find inode");
        return EXT2_RESULT_ERROR;
    }

    struct ext2_inode_descriptor_generic * inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_index);
    if (inode->i_mode & INODE_TYPE_DIR) {
        EXT2_WARN("Trying to read a directory");
        return EXT2_RESULT_ERROR;
    }

    if ((skip+size) > inode->i_size) {
        EXT2_WARN("Trying to read past the end of the file[skip=%d, size=%d, file_size=%d]", skip, size, inode->i_size);
        return EXT2_RESULT_ERROR;
    }


    int64_t read_bytes = ext2_read_inode_bytes(partition, inode_index, destination_buffer, size, skip);
    if (read_bytes == EXT2_READ_FAILED) {
        EXT2_ERROR("File read failed");
        return EXT2_RESULT_ERROR;
    }

    if (read_bytes <= 0) {
        EXT2_ERROR("file read failed");
        return EXT2_RESULT_ERROR;
    }

    if ((uint64_t)read_bytes != size) {
        EXT2_ERROR("Read %d bytes, expected %d", read_bytes, size);
    }

    return EXT2_RESULT_OK;
}

uint32_t ext2_get_inode_index(struct ext2_partition* partition, const char* path) {
    EXT2_INFO("Getting inode index for %s", path);
    uint32_t inode_index = ext2_path_to_inode(partition, path);
    if (inode_index == EXT2_INO_PTI_ERROR) {
        EXT2_WARN("Failed to find inode");
        return EXT2_RESULT_ERROR;
    }

    struct ext2_inode_descriptor_generic * inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_index);
    if (inode == 0) {
        EXT2_ERROR("Failed to read inode");
        return EXT2_RESULT_ERROR;
    }

    return inode_index;
}

uint8_t ext2_write_file(struct ext2_partition * partition, const char * path, uint8_t * source_buffer, uint64_t size, uint64_t skip) {
    EXT2_INFO("Writing file %s", path);

    uint32_t inode_index = ext2_path_to_inode(partition, path);
    if (inode_index == EXT2_INO_PTI_ERROR) {
        EXT2_WARN("File doesn't exist");
        return EXT2_RESULT_ERROR;
    }
    
    EXT2_DEBUG("File created, trying to get inode");
    inode_index = ext2_path_to_inode(partition, path);
    if (inode_index == EXT2_INO_PTI_ERROR) {
        EXT2_ERROR("Failed to get file inode");
        return EXT2_RESULT_ERROR;
    }

    struct ext2_inode_descriptor_generic * inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_index);
    if (inode->i_mode & INODE_TYPE_DIR) {
        EXT2_ERROR("Trying to write to a directory");
        return EXT2_RESULT_ERROR;
    }

    if ((size + skip) > inode->i_size) {
        EXT2_DEBUG("File %s is too small, resizing", path);
        if (ext2_resize_file(partition, inode_index, size + skip) != EXT2_RESULT_OK) {
            EXT2_ERROR("Failed to resize file");
            return EXT2_RESULT_ERROR;
        }
    }

    int64_t write_bytes = ext2_write_inode_bytes(partition, inode_index, source_buffer, size, skip);
    if (write_bytes == EXT2_WRITE_FAILED) {
        EXT2_ERROR("File write failed");
        return EXT2_RESULT_ERROR;
    }

    if (write_bytes <= 0) {
        EXT2_ERROR("file write failed");
        return EXT2_RESULT_ERROR;
    }

    return EXT2_RESULT_OK;
}

uint8_t ext2_delete_file(struct ext2_partition* partition, const char * path) {
    EXT2_INFO("Deleting file %s", path);
    uint32_t inode_index = ext2_path_to_inode(partition, path);
    EXT2_DEBUG("Deleting file %s, inode %d", path, inode_index);
    if (inode_index == EXT2_INO_PTI_ERROR) {
        EXT2_WARN("File doesn't exist");
        return EXT2_RESULT_ERROR;
    }

    EXT2_DEBUG("Deleting file blocks");
    if (ext2_delete_file_blocks(partition, inode_index)) {
        EXT2_ERROR("Failed to deallocate blocks");
        return EXT2_RESULT_ERROR;
    }

    EXT2_DEBUG("Deleting file inode");
    if (ext2_delete_inode(partition, inode_index)) {
        EXT2_ERROR("Failed to delete inode");
        return EXT2_RESULT_ERROR;
    }

    EXT2_DEBUG("Deleting file dentry");
    if (ext2_delete_dentry(partition, path)) {
        EXT2_ERROR("Failed to delete dentry");
        return EXT2_RESULT_ERROR;
    }

    EXT2_DEBUG("Flushing partition");
    ext2_flush_partition(partition);

    return EXT2_RESULT_OK;
}

void ext2_dump_partition(struct ext2_partition* partition) {
    ext2_partition_dump_partition(partition);
}

uint8_t ext2_debug(struct ext2_partition* partition) {
    
    ext2_dump_partition(partition);
    ext2_dump_sb(partition);
    ext2_dump_all_bgs(partition);
    //ext2_dump_all_inodes(partition, "/"); //TODO: Get the root inode name from somewhere
    ext2_dump_inode_bitmap(partition);
    return EXT2_RESULT_OK;
}

void ext2_inhibit_errors(uint8_t t) {
    if (t) {
        INHIBIT_ERRORS();
    } else {
        ALLOW_ERRORS();
    }
}

uint8_t ext2_stacktrace() {
    EXT2_INFO("Printing stacktrace");
    if (ext2_has_errors(EXT2_ERROR_INFO)) {
        kprintf("[EXT2] Stacktrace requested\n");
        ext2_print_errors(EXT2_ERROR_INFO);
        ext2_clear_errors();
    } else {
        kprintf("[EXT2] No errors\n");
    }

    return EXT2_RESULT_OK;
}

uint8_t ext2_errors() {
    return ext2_has_errors(EXT2_ERROR_ERROR);
}

uint16_t ext2_get_file_permissions(struct ext2_partition* partition, const char* path) {
    uint32_t inode_index = ext2_path_to_inode(partition, path);
    if (inode_index == EXT2_INO_PTI_ERROR) {
        EXT2_WARN("File doesn't exist");
        return EXT2_RESULT_ERROR;
    }

    struct ext2_inode_descriptor_generic * inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_index);
    if (inode->i_mode & INODE_TYPE_DIR) {
        EXT2_ERROR("Trying to get permissions of a directory");
        return EXT2_RESULT_ERROR;
    }

    return (inode->i_mode & 0x1FF);
}

/*
Required functions:

create_file ok
read_file ok (no skip)
write_file ok (no skip) (no shrinking)
delete_file ok
modify_file_info (copy + delete)
create_directory ok
delete_directory ok
read_directory ok
modify_directory_info (copy + delete)

create_link
delete_link
*/