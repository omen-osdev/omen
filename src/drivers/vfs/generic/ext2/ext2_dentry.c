#pragma GCC diagnostic ignored "-Wvariadic-macros"

#include "ext2_dentry.h"

#include "ext2_inode.h"
#include "ext2_block.h"

#include "ext2_util.h"
#include "ext2_integrity.h"

#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/libraries/allocators/heap_allocator.h>

#define LIST_MAX 65535 //TODO: Changeme

uint8_t ext2_delete_dentry(struct ext2_partition* partition, const char * path) {
    char * name;
    char * parent_path;
    if (!ext2_path_to_parent_and_name(path, &parent_path, &name)) return 1;
    
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);

    uint32_t parent_inode_number = ext2_path_to_inode(partition, parent_path);
    if (parent_inode_number == EXT2_INO_PTI_ERROR) {
        EXT2_ERROR("Parent directory %s does not exist", parent_path);
        return 1;
    }
    
    struct ext2_inode_descriptor_generic * parent_inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, parent_inode_number);
    if (parent_inode == 0) {
        EXT2_ERROR("Failed to read parent directory %s", parent_path);
        return 1;
    }

    uint32_t target_inode_number = ext2_path_to_inode(partition, path);
    if (target_inode_number == EXT2_INO_PTI_ERROR) {
        EXT2_ERROR("Target file %s does not exist", path);
        return 1;
    }

    EXT2_DEBUG("Deleting file %s, inode: %d parent_inode: %d", path, target_inode_number, parent_inode_number);
    uint8_t deleted = 0;
    if (parent_inode->i_mode & INODE_TYPE_DIR) {
        uint8_t *block_buffer = kmalloc(parent_inode->i_size + block_size);
        if (block_buffer == 0) {
            EXT2_ERROR("Failed to allocate block buffer");
            return 1;
        }

        if (ext2_read_inode_bytes(partition, parent_inode_number, block_buffer, parent_inode->i_size, 0) == EXT2_READ_FAILED) {
            EXT2_ERROR("Failed to read parent directory %s", parent_path);
            return 1;
        }
        
        uint32_t parsed_bytes = 0;
        
        EXT2_DEBUG("Deleting file %s", path);
        struct ext2_directory_entry* previous_entry = 0;
        while (parsed_bytes < parent_inode->i_size) {
            struct ext2_directory_entry *entry = (struct ext2_directory_entry *) (block_buffer + parsed_bytes);
            if (entry->inode == target_inode_number) {
                entry->inode = 0;
                entry->name_len = 0;
                entry->file_type = 0;
                entry->name[0] = 0;
                if (previous_entry) {
                    previous_entry->rec_len += entry->rec_len;
                }
                entry->rec_len = 0;

                EXT2_DEBUG("Deleted dentry");
                deleted = 1;

                break;
            }
            previous_entry = entry;
            parsed_bytes += entry->rec_len;
        }
        if (deleted)
            if (ext2_write_inode_bytes(partition, parent_inode_number, block_buffer, parent_inode->i_size, 0) == EXT2_WRITE_FAILED) {
                EXT2_ERROR("Failed to write parent directory %s", parent_path);
                return 1;
            }

        kfree(block_buffer);
    }

    kfree(parent_path);
    kfree(name);
    
    if (!deleted) {
        EXT2_ERROR("Failed to delete file %s", path);
        return 1; 
    }

    return 0;  

}

/*
uint8_t ext2_dentry_get_dentry(struct ext2_partition* partition, const char* parent_path, const char* name, struct ext2_directory_entry* target_entry) {
    uint32_t inode_number = ext2_path_to_inode(partition, parent_path);
    if (inode_number == EXT2_INO_PTI_ERROR) {
        EXT2_ERROR("Directory %s does not exist", parent_path);
        return 1;
    }

    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    struct ext2_inode_descriptor_generic * root_inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_number);
    if (root_inode == 0) {
        EXT2_ERROR("Failed to read directory %s", parent_path);
        return 1;
    }

    if (root_inode->i_mode & INODE_TYPE_DIR) {
        uint8_t *block_buffer[1024];
        uint16_t block_buffer_index = 0;

        block_buffer[block_buffer_index] = kmalloc(block_size);

        uint32_t read_bytes = 0;
        uint32_t parsed_bytes = 0;
        uint32_t entry_count = 0;

        do {
            if (ext2_read_inode_bytes(partition, inode_number, block_buffer[block_buffer_index], 1024, read_bytes) == EXT2_READ_FAILED) {
                EXT2_ERROR("Failed to read directory %s", parent_path);
                return 1;
            }

            parsed_bytes = 0;
            struct ext2_directory_entry *entry = 0;
            do {
                entry = (struct ext2_directory_entry *) (block_buffer[block_buffer_index] + parsed_bytes);
                if (entry->inode == 0) {
                    // No more entries in this block
                    break;
                }
                if (entry->rec_len == 0) {
                    EXT2_ERROR("Invalid entry in directory %s", parent_path);
                    kfree(block_buffer[block_buffer_index]);
                    return 1;
                }

                char buffer[EXT2_NAME_LEN + 1] = {0};
                strncpy(buffer, entry->name, entry->name_len);
                buffer[entry->name_len] = '\0';
                EXT2_DEBUG("Checking entry: %s", buffer);
                if (strncmp(buffer, name, strlen(name)) == 0) {
                    EXT2_DEBUG("Found entry: %s", buffer);
                    memcpy(target_entry, entry, sizeof(struct ext2_directory_entry));
                    for (uint16_t i = 0; i < block_buffer_index; i++) {
                        kfree(block_buffer[i]);
                    }
                    return 0;
                }

                entry_count++;
                parsed_bytes += entry->rec_len;
                read_bytes += entry->rec_len;
            } while (parsed_bytes + entry->rec_len < block_size);
            block_buffer_index++;
            if (block_buffer_index >= 1024) {
                EXT2_ERROR("Too many blocks read for directory %s", parent_path);
                for (uint16_t i = 0; i < block_buffer_index; i++) {
                    kfree(block_buffer[i]);
                }
                return 1;
            }
        } while (1);

        EXT2_WARN("Entry %s not found in directory %s", name, parent_path);
        memset(target_entry, 0, sizeof(struct ext2_directory_entry));
        target_entry->inode = EXT2_INO_PTI_ERROR; // Indicate not found
        target_entry->rec_len = 0;
        target_entry->name_len = 0;
        target_entry->file_type = 0;
        target_entry->name[0] = '\0'; // Clear name
        EXT2_ERROR("[EXT2] Entry %s not found in directory %s\n", name, parent_path);
        for (uint16_t i = 0; i < block_buffer_index; i++) {
            kfree(block_buffer[i]);
        }
        return 1;
    }
}
*/
uint8_t ext2_dentry_get_dentry(struct ext2_partition* partition, const char* parent_path, const char* name, struct ext2_directory_entry* entry) {
    uint32_t inode_number = ext2_path_to_inode(partition, parent_path);
    if (inode_number == EXT2_INO_PTI_ERROR) {
        EXT2_ERROR("Directory %s does not exist", parent_path);
        return 1;
    }

    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    struct ext2_inode_descriptor_generic * root_inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_number);
    if (root_inode == 0) {
        EXT2_ERROR("Failed to read directory %s", parent_path);
        return 1;
    }

    if (root_inode->i_mode & INODE_TYPE_DIR) {
        uint8_t *block_buffer = kmalloc(root_inode->i_size + block_size);
        if (block_buffer == 0) {
            EXT2_ERROR("Failed to allocate block buffer");
            return 1;
        }

        if (ext2_read_inode_bytes(partition, inode_number, block_buffer, root_inode->i_size, 0) == EXT2_READ_FAILED) {
            EXT2_ERROR("Failed to read directory %s", parent_path);
            return 1;
        }

        uint32_t parsed_bytes = 0;
        uint32_t list_count = 0;
        while (list_count < LIST_MAX) {
            struct ext2_directory_entry *centry = (struct ext2_directory_entry *) (block_buffer + parsed_bytes);
            if (centry->rec_len == 0 || centry->inode == 0) break;
            if (centry->inode != 0) {
                char buffer[EXT2_NAME_LEN + 1] = {0};
                strncpy(buffer, centry->name, centry->name_len);
                buffer[centry->name_len] = '\0';
                EXT2_DEBUG("Checking entry: %s", buffer);
                if (strncmp(buffer, name, strlen(name)) == 0) {
                    EXT2_DEBUG("Found entry: %s", buffer);
                    memcpy(entry, centry, sizeof(struct ext2_directory_entry));
                    kfree(block_buffer);
                    return 0;
                }
            }
            parsed_bytes += centry->rec_len;
            list_count++;
        }
        EXT2_WARN("Entry %s not found in directory %s", name, parent_path);
        memset(entry, 0, sizeof(struct ext2_directory_entry));
        entry->inode = EXT2_INO_PTI_ERROR; // Indicate not found
        entry->rec_len = 0;
        entry->name_len = 0;
        entry->file_type = 0;
        entry->name[0] = '\0'; // Clear name
        EXT2_ERROR("[EXT2] Entry %s not found in directory %s\n", name, parent_path);
        kfree(block_buffer);
    }

    return 1;
}

void ext2_list_dentry(struct ext2_partition* partition, const char * path) {
    uint32_t inode_number = ext2_path_to_inode(partition, path);
    if (inode_number == EXT2_INO_PTI_ERROR) {
        EXT2_ERROR("Directory %s does not exist", path);
        return;
    }

    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    struct ext2_inode_descriptor_generic * root_inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_number);
    if (root_inode == 0) {
        EXT2_ERROR("Failed to read directory %s", path);
        return;
    }

    if (root_inode->i_mode & INODE_TYPE_DIR) {
        uint8_t *block_buffer = kmalloc(root_inode->i_size + block_size);
        if (block_buffer == 0) {
            EXT2_ERROR("Failed to allocate block buffer");
            return;
        }

        if (ext2_read_inode_bytes(partition, inode_number, block_buffer, root_inode->i_size, 0) == EXT2_READ_FAILED) {
            EXT2_ERROR("Failed to read directory %s", path);
            return;
        }

        uint32_t parsed_bytes = 0;
        uint32_t list_count = 0;
        DBG_INFO("[EXT2] Directory listing for %s\n", path);
        while (parsed_bytes < root_inode->i_size && list_count < LIST_MAX) {
            struct ext2_directory_entry *entry = (struct ext2_directory_entry *) (block_buffer + parsed_bytes);
            DBG_INFO("[EXT2] ino: %d rec_len: %d name_len: %d file_type: %d name: %s\n", entry->inode, entry->rec_len, entry->name_len, entry->file_type, entry->name);
            parsed_bytes += entry->rec_len;
            list_count++;
        }
       
        kfree(block_buffer);
    }
}

/*
uint32_t ext2_get_all_dirs(struct ext2_partition* partition, const char* parent_path, struct ext2_directory_entry** entries) {
    uint32_t inode_number = ext2_path_to_inode(partition, parent_path);
    if (inode_number == EXT2_INO_PTI_ERROR) {
        EXT2_ERROR("Directory %s does not exist", parent_path);
        return 0;
    }

    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    struct ext2_inode_descriptor_generic * root_inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_number);
    if (root_inode == 0) {
        EXT2_ERROR("Failed to read directory %s", parent_path);
        return 0;
    }

    if (root_inode->i_mode & INODE_TYPE_DIR) {
        uint8_t * block_buffer[1024];
        uint16_t block_buffer_index = 0;
        
        block_buffer[block_buffer_index] = kmalloc(block_size);

        uint32_t read_bytes = 0;
        uint32_t parsed_bytes = 0;
        uint32_t entry_count = 0;

        do {
            if (ext2_read_inode_bytes(partition, inode_number, block_buffer[block_buffer_index], block_size, read_bytes) == EXT2_READ_FAILED) {
                EXT2_ERROR("Failed to read directory %s", parent_path);
                return 0;
            }

            parsed_bytes = 0;
            struct ext2_directory_entry *entry = 0;
            do {

                entry = (struct ext2_directory_entry *) (block_buffer[block_buffer_index] + parsed_bytes);
                if (entry->inode == 0) {
                    // No more entries in this block
                    break;
                }
                if (entry->rec_len == 0) {
                    EXT2_ERROR("Invalid entry in directory %s", parent_path);
                    kfree(block_buffer[block_buffer_index]);
                    return 0;
                }

                (*entries)[entry_count] = *entry;

                entry_count++;
                parsed_bytes += entry->rec_len;
                read_bytes += entry->rec_len;
            } while (parsed_bytes + entry->rec_len < block_size);
            block_buffer_index++;
            if (block_buffer_index >= 1024) {
                EXT2_ERROR("Too many blocks in directory %s", parent_path);
                for (uint16_t i = 0; i < block_buffer_index; i++) {
                    kfree(block_buffer[i]);
                }
                return 0;
            }
        } while (1);

        return entry_count;
    }

    EXT2_ERROR("Inode %d is not a directory", inode_number);
    return 0;
}
*/

uint32_t ext2_get_all_dirs(struct ext2_partition* partition, const char* parent_path, struct ext2_directory_entry** entries) {
    uint32_t inode_number = ext2_path_to_inode(partition, parent_path);
    if (inode_number == EXT2_INO_PTI_ERROR) {
        EXT2_ERROR("Directory %s does not exist", parent_path);
        return 0;
    }

    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    struct ext2_inode_descriptor_generic * root_inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_number);
    if (root_inode == 0) {
        EXT2_ERROR("Failed to read directory %s", parent_path);
        return 0;
    }

    if (root_inode->i_mode & INODE_TYPE_DIR) {
        uint8_t *block_buffer = kmalloc(root_inode->i_size + block_size);
        if (block_buffer == 0) {
            EXT2_ERROR("Failed to allocate block buffer");
            return 0;
        }

        if (ext2_read_inode_bytes(partition, inode_number, block_buffer, root_inode->i_size, 0) == EXT2_READ_FAILED) {
            EXT2_ERROR("Failed to read directory %s", parent_path);
            return 0;
        }

        uint32_t parsed_bytes = 0;
        uint32_t entry_count = 0;
        while (parsed_bytes < root_inode->i_size) {
            struct ext2_directory_entry *entry = (struct ext2_directory_entry *) (block_buffer + parsed_bytes);
            if (entry->inode != 0) {
                entry_count++;
            }
            parsed_bytes += entry->rec_len;
        }
        *entries = kmalloc(sizeof(struct ext2_directory_entry) * entry_count);
        if (*entries == 0) {
            EXT2_ERROR("Failed to allocate entries buffer");
            return 0;
        }

        parsed_bytes = 0;
        entry_count = 0;
        while (parsed_bytes < root_inode->i_size) {
            struct ext2_directory_entry *entry = (struct ext2_directory_entry *) (block_buffer + parsed_bytes);
            if (entry->inode != 0) {
                (*entries)[entry_count] = *entry;
                entry_count++;
            }
            parsed_bytes += entry->rec_len;
        }
        kfree(block_buffer);
        return entry_count;
    }
    return 0;

}

uint8_t ext2_operate_on_dentry(struct ext2_partition* partition, const char* path, uint8_t (*callback)(struct ext2_partition* partition, uint32_t inode_entry)) {
    uint32_t inode_number = ext2_path_to_inode(partition, path);
    if (inode_number == EXT2_INO_PTI_ERROR) {
        EXT2_ERROR("Directory %s does not exist", path);
        return 0;
    }

    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    struct ext2_inode_descriptor_generic * root_inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_number);
    if (root_inode == 0) {
        EXT2_ERROR("Failed to read directory %s", path);
        return 0;
    }

    if (root_inode->i_mode & INODE_TYPE_DIR) {
        uint8_t *block_buffer = kmalloc(root_inode->i_size + block_size);
        if (block_buffer == 0) {
            EXT2_ERROR("Failed to allocate block buffer");
            return 0;
        }

        if (ext2_read_inode_bytes(partition, inode_number, block_buffer, root_inode->i_size, 0) == EXT2_READ_FAILED) {
            EXT2_ERROR("Failed to read directory %s", path);
            return 0;
        }

        uint32_t parsed_bytes = 0;
        
        while (parsed_bytes < root_inode->i_size) {
            struct ext2_directory_entry *entry = (struct ext2_directory_entry *) (block_buffer + parsed_bytes);
            if (callback(partition, entry->inode)) {
                EXT2_ERROR("Failed to operate on dentry");
                kfree(block_buffer);
                return 1;
            }
            parsed_bytes += entry->rec_len;
        }
       
        kfree(block_buffer);
    }

    return 0;
}

uint8_t ext2_initialize_directory(struct ext2_partition* partition, uint32_t inode_number, uint32_t parent_inode_number) {
    struct ext2_inode_descriptor_generic * root_inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_number);
    if (root_inode == 0) {
        EXT2_ERROR("Failed to read directory inode");
        return 1;
    }

    //Create . and .. entries
    struct ext2_directory_entry entries[2] = {
        {
            .inode = inode_number,
            .rec_len = 0,
            .name_len = 1,
            .file_type = EXT2_DIR_TYPE_DIRECTORY,
            .name = "."
        },
        {
            .inode = parent_inode_number,
            .rec_len = 0,
            .name_len = 2,
            .file_type = EXT2_DIR_TYPE_DIRECTORY,
            .name = ".."
        }
    };

    uint32_t entry_size_first = sizeof(struct ext2_directory_entry) + entries[0].name_len - EXT2_NAME_LEN;
    entries[0].rec_len = (entry_size_first + 3) & ~3;

    uint32_t entry_size_second = sizeof(struct ext2_directory_entry) + entries[1].name_len - EXT2_NAME_LEN;
    entries[1].rec_len = 1024 - entries[0].rec_len;

    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    uint8_t *block_buffer = kmalloc(block_size);
    if (block_buffer == 0) {
        EXT2_ERROR("Failed to allocate block buffer");
        return 1;
    }

    if (memset(block_buffer, 0, block_size) == 0) {
        EXT2_ERROR("Failed to clear block buffer");
        kfree(block_buffer);
        return 1;
    }

    if (ext2_read_inode_bytes(partition, inode_number, block_buffer, root_inode->i_size, 0) == EXT2_READ_FAILED) {
        EXT2_ERROR("Failed to read directory inode");
        kfree(block_buffer);
        return 1;
    }

    if (memcpy(block_buffer, &entries[0], entry_size_first) == 0) {
        EXT2_ERROR("Failed to copy first directory entry");
        kfree(block_buffer);
        return 1;
    }

    if (memcpy(block_buffer + entries[0].rec_len, &entries[1], entry_size_second) == 0) {
        EXT2_ERROR("Failed to copy second directory entry");
        kfree(block_buffer);
        return 1;
    }


    if (ext2_write_inode_bytes(partition, inode_number, block_buffer, root_inode->i_size, 0) == EXT2_WRITE_FAILED) {
        EXT2_ERROR("Failed to write directory entry");
        kfree(block_buffer);
        return 1;
    }

    kfree(block_buffer);
    return 0;
}

int64_t ext2_create_directory_entry(struct ext2_partition* partition, uint32_t inode_number, uint32_t child_inode, const char* name, uint32_t type) {
    EXT2_DEBUG("File name: %s, Parent inode: %d, Type: %d", name, inode_number, type);

    struct ext2_inode_descriptor_generic * root_inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_number);
    if (root_inode == 0) {
        EXT2_ERROR("Failed to read directory inode");
        return 1;
    }

    struct ext2_directory_entry child_entry = {
        .inode = child_inode,
        .rec_len = 0,
        .name_len = strlen(name),
        .file_type = type
    };

    uint32_t entry_size = sizeof(struct ext2_directory_entry) + child_entry.name_len - EXT2_NAME_LEN;
    child_entry.rec_len = (entry_size + 3) & ~3;

    if (root_inode->i_mode & INODE_TYPE_DIR) {
        uint8_t *block_buffer = kmalloc(root_inode->i_size);
        if (block_buffer == 0) {
            EXT2_ERROR("Failed to allocate block buffer");
            return 1;
        }

        if (ext2_read_inode_bytes(partition, inode_number, block_buffer, root_inode->i_size, 0) == EXT2_READ_FAILED) {
            EXT2_ERROR("Failed to read directory inode");
            kfree(block_buffer);
            return 1;
        }

        uint32_t parsed_bytes = 0;
        
        struct ext2_directory_entry *entry = 0;
        while (parsed_bytes < root_inode->i_size) {
            entry = (struct ext2_directory_entry *) (block_buffer + parsed_bytes);
            parsed_bytes += entry->rec_len;
        }

        if (entry == 0) {
            EXT2_ERROR("No directory entries");
            kfree(block_buffer);
            return 1;
        }

        uint32_t entry_size = sizeof(struct ext2_directory_entry) + entry->name_len - EXT2_NAME_LEN;
        uint32_t entry_size_aligned = (entry_size + 3) & ~3;

        if (entry_size_aligned + child_entry.rec_len > entry->rec_len) {
            EXT2_ERROR("No space for directory entry");
            kfree(block_buffer);
            return 1;
        }

        uint32_t original_rec_len = entry->rec_len;
        entry->rec_len = entry_size_aligned;
        child_entry.rec_len = original_rec_len - entry_size_aligned;

        if (memcpy(block_buffer + parsed_bytes - original_rec_len, entry, entry_size) == 0) {
            EXT2_ERROR("Failed to copy directory entry");
            kfree(block_buffer);
            return 1;
        }

        if (memcpy(block_buffer + parsed_bytes - original_rec_len + entry_size_aligned, &child_entry, sizeof(struct ext2_directory_entry) - EXT2_NAME_LEN) == 0) {
            EXT2_ERROR("Failed to copy directory entry");
            kfree(block_buffer);
            return 1;
        }

        if (memcpy(block_buffer + parsed_bytes - original_rec_len + entry_size_aligned + sizeof(struct ext2_directory_entry) - EXT2_NAME_LEN, name, child_entry.name_len) == 0) {
            EXT2_ERROR("Failed to copy directory entry");
            kfree(block_buffer);
            return 1;
        }

        if (ext2_write_inode_bytes(partition, inode_number, block_buffer, root_inode->i_size, 0) == EXT2_WRITE_FAILED) {
            EXT2_ERROR("Failed to write directory entry");
            kfree(block_buffer);
            return 1;
        }

        kfree(block_buffer);
        EXT2_DEBUG("Directory entry created");
        return 0;
    } else {
        EXT2_ERROR("Not a directory");
        return 1;
    }
}