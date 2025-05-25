#pragma GCC diagnostic ignored "-Wvariadic-macros"

#include "ext2_inode.h"

#include "ext2_dentry.h"
#include "ext2_block.h"

#include "ext2_util.h"
#include "ext2_integrity.h"

#include <omen/libraries/std/stdint.h>
#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <disk/disk_interface.h>

#define EXT2_ROOT_INO_INDEX     2

void ext2_dump_inode_bitmap(struct ext2_partition * partition) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    uint32_t block_group_count = partition->group_number;
    
    kprintf("[EXT2] Dumping inode bitmaps: %d\n", block_group_count);
    for (uint32_t i = 0; i < block_group_count; i++) {
        struct ext2_block_group_descriptor * bgd = (struct ext2_block_group_descriptor *)&partition->gd[i];
        uint8_t * bitmap = (uint8_t*)ext2_buffer_for_size(block_size, block_size);
        if (bitmap == 0) {
            EXT2_ERROR("Failed to allocate memory for inode bitmap");
            return;
        }

        if (ext2_read_block(partition, bgd->bg_inode_bitmap, bitmap) != 1) {
            EXT2_ERROR("Failed to read inode bitmap %d\n", i);
            continue;
        }

        kprintf("[EXT2] Inode bitmap for block group %d\n", i);
        for (uint32_t j = 0; j < block_size; j++) {
            kprintf("%02x ", bitmap[j]);
        }
        kprintf("\n");

        kfree(bitmap);
    }
}

uint32_t ext2_allocate_inode(struct ext2_partition * partition) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);

    //ext2_operate_on_bg(partition, ext2_dump_bg);
    //ext2_dump_inode_bitmap(partition);

    int32_t ext2_block_group_id = ext2_operate_on_bg(partition, ext2_bg_has_free_inodes);
    if (ext2_block_group_id == -1) {
        EXT2_WARN("No block group with free inodes");
        return 0;
    }

    struct ext2_block_group_descriptor * bgd = (struct ext2_block_group_descriptor *)&partition->gd[ext2_block_group_id];

    if (bgd == 0) {
        EXT2_ERROR("No bg with free inodes");
        return 0;
    }

    EXT2_DEBUG("Found bg with free inodes");

    uint8_t * inode_bitmap = (uint8_t*)ext2_buffer_for_size(block_size, block_size);
    if (inode_bitmap == 0) {
        EXT2_ERROR("Failed to allocate memory for inode bitmap");
        return 0;
    }

    if (ext2_read_block(partition, bgd->bg_inode_bitmap, inode_bitmap) != 1) {
        EXT2_ERROR("Failed to read inode bitmap");
        kfree(inode_bitmap);
        return 0;
    }

    uint32_t inode_number = 0;
    for (uint32_t i = 0; i < block_size; i++) {
        if (inode_bitmap[i] != 0xFF) {
            for (uint32_t j = 0; j < 8; j++) {
                if ((inode_bitmap[i] & (1 << j)) == 0) {
                    inode_number = i * 8 + j + 1;
                    break;
                }
            }
            break;
        }
    }

    //0f 00001111
    //2f 00101111
    if (inode_number == 0) {
        EXT2_WARN("No free inodes");
        kfree(inode_bitmap);
        return 0;
    }

    EXT2_DEBUG("Found free inode: %d", inode_number);

    inode_bitmap[(inode_number-1) / 8] |= 1 << (inode_number-1) % 8;

    if (ext2_write_block(partition, bgd->bg_inode_bitmap, inode_bitmap) <= 0) {
        EXT2_ERROR("Failed to write inode bitmap");
        kfree(inode_bitmap);
        return 0;
    }

    bgd->bg_free_inodes_count--;

    //Update superblock
    struct ext2_superblock_extended * sb = (struct ext2_superblock_extended*)partition->sb;
    ((struct ext2_superblock*)sb)->s_free_inodes_count--;

    //Update partition
    partition->sb = sb;
    partition->gd[ext2_block_group_id] = *bgd;

    ext2_flush_required(partition);
    kfree(inode_bitmap);

    return inode_number;
}

void ext2_debug_print_file_inode(struct ext2_partition* partition, uint32_t inode_number) {
    if (inode_number == 0) {
        EXT2_ERROR("Failed to get inode number for %d", inode_number);
        return;
    }

    struct ext2_inode_descriptor * inode = ext2_read_inode(partition, inode_number);
    if (inode == 0) {
        EXT2_ERROR("Failed to read inode %d", inode_number);
    }

    ext2_print_inode((struct ext2_inode_descriptor_generic*)inode);
}

struct ext2_inode_descriptor * ext2_initialize_inode(struct ext2_partition* partition, uint32_t inode_number, uint32_t type, uint32_t permissions) {
    struct ext2_inode_descriptor * inode_descriptor = ext2_read_inode(partition, inode_number);
    if (inode_descriptor == 0) {
        EXT2_ERROR("Failed to read inode %d", inode_number);
        return 0;
    }

    struct ext2_inode_descriptor_generic * inode = &(inode_descriptor->id);
    if (inode == 0) {
        EXT2_ERROR("Failed to allocate memory for inode");
        return 0;
    }

    uint32_t unique_value = ext2_get_unique_id();
    if (unique_value == 0) {
        EXT2_ERROR("Failed to get unique id");
        return 0;
    }

    inode->i_mode = type | permissions;
    inode->i_uid = ext2_get_uid();   
    inode->i_size = 0;
    inode->i_atime = ext2_get_current_epoch(); 
    inode->i_ctime = inode->i_atime;
    inode->i_mtime = inode->i_atime;
    inode->i_dtime = 0; 
    inode->i_gid = ext2_get_gid();
    inode->i_links_count = 1;
    inode->i_sectors = 0;
    inode->i_flags = 0;
    inode->i_osd1 = 0;    
    
    if (memset(inode->i_block, 0, sizeof(uint32_t) * 15) == 0) {
        EXT2_ERROR("Failed to initialize inode block");
        return 0;
    }

    inode->i_generation = unique_value;
    inode->i_file_acl = 0;
    inode->i_dir_acl = 0; 
    inode->i_faddr = 0;  

    return inode_descriptor;
}

uint8_t* ext2_read_inode_bitmap(struct ext2_partition* partition, uint32_t inode_number) {
    struct ext2_superblock * superblock = (struct ext2_superblock*)partition->sb;
    uint32_t block_size = 1024 << superblock->s_log_block_size;
    uint32_t inode_group = (inode_number - 1 ) / superblock->s_inodes_per_group;
    uint32_t sectors_per_block = block_size / partition->sector_size;

    uint32_t inode_bitmap_block = partition->gd[inode_group].bg_inode_bitmap;
    uint32_t inode_bitmap_lba = (inode_bitmap_block * block_size) / partition->sector_size;
    uint8_t * inode_bitmap_buffer = kmalloc(block_size);
    if (inode_bitmap_buffer == 0) {
        EXT2_ERROR("Failed to allocate memory for inode bitmap");
        return 0;
    }

    if (!read_disk(partition->disk, inode_bitmap_buffer, partition->lba + inode_bitmap_lba, sectors_per_block*partition->sector_size)) {
        EXT2_ERROR("Inode read failed");
        kfree(inode_bitmap_buffer);
        return 0;
    }

    return inode_bitmap_buffer;
}

uint8_t ext2_write_inode_bitmap(struct ext2_partition* partition, uint32_t inode_number, uint8_t* inode_bitmap) {
    struct ext2_superblock * superblock = (struct ext2_superblock*)partition->sb;
    uint32_t block_size = 1024 << superblock->s_log_block_size;
    uint32_t inode_group = (inode_number - 1 ) / superblock->s_inodes_per_group;
    uint32_t sectors_per_block = block_size / partition->sector_size;
    
    uint32_t inode_bitmap_block = partition->gd[inode_group].bg_inode_bitmap;
    uint32_t inode_bitmap_lba = (inode_bitmap_block * block_size) / partition->sector_size;
    if (!write_disk(partition->disk, inode_bitmap, partition->lba + inode_bitmap_lba, sectors_per_block)) {
        EXT2_ERROR("Inode write failed");
        return 1;
    }

    return 0;
}

uint8_t ext2_write_inode(struct ext2_partition* partition, uint32_t inode_number, struct ext2_inode_descriptor* inode) {
    struct ext2_superblock * superblock = (struct ext2_superblock*)partition->sb;
    struct ext2_superblock_extended * superblock_extended = (struct ext2_superblock_extended*)partition->sb;

    uint32_t block_size = 1024 << superblock->s_log_block_size;
    uint32_t sectors_per_block = DIVIDE_ROUNDED_UP(block_size, partition->sector_size);
    uint32_t inode_size = (superblock->s_rev_level < 1) ? 128 : superblock_extended->s_inode_size;

    uint32_t inode_group = (inode_number - 1 ) / superblock->s_inodes_per_group;
    uint32_t inode_index = (inode_number - 1 ) % superblock->s_inodes_per_group;
    uint32_t inode_block = (inode_index * inode_size) / (block_size);
    uint32_t inode_table_block = partition->gd[inode_group].bg_inode_table;
    uint32_t inode_table_lba = (inode_table_block * block_size) / partition->sector_size;
    uint8_t * root_inode_buffer = kmalloc(block_size);
    if (root_inode_buffer == 0) {
        EXT2_ERROR("Failed to allocate memory for root inode");
        return 1;
    }

    if (!read_disk(partition->disk, root_inode_buffer, partition->lba + inode_table_lba + inode_block*sectors_per_block, sectors_per_block*partition->sector_size)) {
        EXT2_ERROR("Root inode read failed");
        kfree(root_inode_buffer);
        return 1;
    }

    uint32_t inode_offset = (inode_index * inode_size) % block_size;
    if (memcpy(root_inode_buffer + inode_offset, inode, inode_size) == 0) {
        EXT2_ERROR("Failed to copy root inode");
        kfree(root_inode_buffer);
        return 1;
    }

    if (!write_disk(partition->disk, root_inode_buffer, partition->lba + inode_table_lba + inode_block*sectors_per_block, sectors_per_block*partition->sector_size)) {
        EXT2_ERROR("Root inode write failed");
        kfree(root_inode_buffer);
        return 1;
    }

    kfree(root_inode_buffer);

    return 0;
}

struct ext2_inode_descriptor * ext2_read_inode(struct ext2_partition* partition, uint32_t inode_number) {
    
    struct ext2_superblock * superblock = (struct ext2_superblock*)partition->sb;
    struct ext2_superblock_extended * superblock_extended = (struct ext2_superblock_extended*)partition->sb;

    uint32_t block_size = 1024 << superblock->s_log_block_size;
    uint32_t sectors_per_block = DIVIDE_ROUNDED_UP(block_size, partition->sector_size);
    uint32_t inode_size = (superblock->s_rev_level < 1) ? 128 : superblock_extended->s_inode_size;

    uint32_t inode_group = (inode_number - 1 ) / superblock->s_inodes_per_group;
    uint32_t inode_index = (inode_number - 1 ) % superblock->s_inodes_per_group;
    uint32_t inode_block = (inode_index * inode_size) / (block_size);
    uint32_t inode_table_block = partition->gd[inode_group].bg_inode_table;
    uint32_t inode_table_lba = (inode_table_block * block_size) / partition->sector_size;
    uint8_t * root_inode_buffer = kmalloc(block_size);
    EXT2_INFO("Reading inode %d Grp: %d Idx: %d Blk: %d, Tbl_blk: %d Tbl_lba: %d", inode_number, inode_group, inode_index, inode_block, inode_table_block, inode_table_lba);
    if (root_inode_buffer == 0) {
        EXT2_ERROR("Failed to allocate memory for root inode");
        return 0;
    }

    if (!read_disk(partition->disk, root_inode_buffer, partition->lba + inode_table_lba + inode_block*sectors_per_block, sectors_per_block*partition->sector_size)) {
        EXT2_ERROR("Root inode read failed");
        kfree(root_inode_buffer);
        return 0;
    }

    struct ext2_inode_descriptor * inode = kmalloc(inode_size);
    if (inode == 0) {
        EXT2_ERROR("Failed to allocate memory for root inode");
        kfree(root_inode_buffer);
        return 0;
    }

    if (memcpy(inode, root_inode_buffer + (((inode_index * inode_size) % block_size)), inode_size) == 0) {
        EXT2_ERROR("Failed to copy root inode");
        kfree(root_inode_buffer);
        kfree(inode);
        return 0;
    }

    kfree(root_inode_buffer);

    return inode;
}

void ext2_print_inode(struct ext2_inode_descriptor_generic* inode) {
    char atime[32];
    char ctime[32];
    char mtime[32];
    char dtime[32];

    epoch_to_date(atime, inode->i_atime);
    epoch_to_date(ctime, inode->i_ctime);
    epoch_to_date(mtime, inode->i_mtime);
    epoch_to_date(dtime, inode->i_dtime);

    kprintf("[EXT2] Inode dump start ----\n");
    kprintf("i_mode: %d\n", inode->i_mode);
    kprintf("i_uid: %d\n", inode->i_uid);
    kprintf("i_size: %d\n", inode->i_size);
    kprintf("i_atime: %s\n", atime);
    kprintf("i_ctime: %s\n", ctime);
    kprintf("i_mtime: %s\n", mtime);
    kprintf("i_dtime: %s\n", dtime);
    kprintf("i_gid: %d\n", inode->i_gid);
    kprintf("i_links_count: %d\n", inode->i_links_count);
    kprintf("i_sectors: %d\n", inode->i_sectors);
    kprintf("i_flags: %d\n", inode->i_flags);
    kprintf("i_osd1: %d\n", inode->i_osd1);
    kprintf("i_generation: %d\n", inode->i_generation);
    kprintf("i_file_acl: %d\n", inode->i_file_acl);
    kprintf("i_dir_acl: %d\n", inode->i_dir_acl);
    kprintf("i_faddr: %d\n", inode->i_faddr);
    kprintf("[EXT2] Inode dump end ----\n");

}

int32_t ext2_inode_from_path_and_parent(struct ext2_partition* partition, uint32_t parent_inode, const char* path) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);

    if (path != 0 && path[0] == '\0') {
        return parent_inode; // Return the parent inode if the path is root
    }
    
    struct ext2_inode_descriptor_generic * root_inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, parent_inode);
    if (root_inode == 0) {
        EXT2_WARN("Failed to read root inode");
        return EXT2_INO_PAP_ERROR;
    }

    if (root_inode->i_mode & INODE_TYPE_DIR) {
        uint8_t *block_buffer = kmalloc(root_inode->i_size + block_size);
        memset(block_buffer, 0, root_inode->i_size + block_size);
        if (block_buffer == 0) {
            EXT2_ERROR("Failed to allocate memory for block buffer");
            kfree(root_inode);
            return EXT2_INO_PAP_ERROR;
        }

        if (ext2_read_inode_bytes(partition, parent_inode, block_buffer, root_inode->i_size, 0) == EXT2_READ_FAILED) {
            EXT2_ERROR("Failed to read root inode");
            kfree(root_inode);
            kfree(block_buffer);
            return EXT2_INO_PAP_ERROR;
        }

        uint32_t parsed_bytes = 0;

        while (parsed_bytes < root_inode->i_size) {
            struct ext2_directory_entry *entry = (struct ext2_directory_entry *) (block_buffer + parsed_bytes);
            if (strncmp(entry->name, path, entry->name_len) == 0) {
                uint32_t inode = entry->inode;
                EXT2_INFO("Returning inode %d", inode);
                kfree(block_buffer);
                return inode;
            }
            parsed_bytes += entry->rec_len;
        }
        
        kfree(block_buffer);
    }

    EXT2_WARN("Failed to find inode");
    return EXT2_INO_PAP_NOTFOUND;
}

uint32_t ext2_path_to_inode(struct ext2_partition* partition, const char * path) {
    char * path_copy = kmalloc(strlen(path) + 1);
    if (path_copy == 0) {
        EXT2_ERROR("Failed to allocate memory for path copy");
        return EXT2_INO_PTI_ERROR;
    }

    memset(path_copy, 0, strlen(path) + 1);
    strncpy(path_copy, path, strlen(path) + 1);
    char * token = strtok(path_copy, "/");
    int32_t inode_index_i32 = EXT2_ROOT_INO_INDEX;
    uint32_t inode_index = EXT2_ROOT_INO_INDEX;
    while (token != 0) {
        EXT2_INFO("Previous index: %d, current token: %s", inode_index, token);
        inode_index_i32 = ext2_inode_from_path_and_parent(partition, inode_index, token);
        if (inode_index_i32 == EXT2_INO_PAP_ERROR || inode_index_i32 == EXT2_INO_PAP_NOTFOUND) {
            EXT2_ERROR("Failed to get inode from path and parent prev: %d, token: %s", inode_index, token);
            return EXT2_INO_PTI_ERROR;
        }

        inode_index = (uint32_t) inode_index_i32;
        EXT2_INFO("Next index: %d", inode_index_i32);
        token = strtok(0, "/");
    }
    if (inode_index == 0) {
        EXT2_ERROR("[CRITIAL] SILENT ERROR Inode Zer0 %s", path);
        panic("Inode Zer0");
    }
    return inode_index;
}

uint32_t ext2_load_indirect_block_list(struct ext2_partition* partition, uint32_t * block_buffer, uint32_t block) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    uint32_t * indirect_block = kmalloc(block_size);
    if (indirect_block == 0) {
        EXT2_ERROR("Failed to allocate memory for indirect block");
        return EXT2_READ_FAILED;
    }

    if (ext2_read_block(partition, block, (uint8_t*)indirect_block) == EXT2_READ_FAILED) {
        EXT2_ERROR("Failed to read indirect block");
        kfree(indirect_block);
        return EXT2_READ_FAILED;
    }

    uint32_t blocks_read = 0;
    for (uint32_t i = 0; i < block_size / 4; i++) {
        if (indirect_block[i] == 0 || indirect_block[i] == INODE_BLOCK_END) {
            break;
        }

        block_buffer[blocks_read] = indirect_block[i];
        blocks_read++;
    }

    kfree(indirect_block);
    return blocks_read;
}

uint32_t ext2_load_double_indirect_block_list(struct ext2_partition* partition, uint32_t * block_buffer, uint32_t block) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    uint32_t * indirect_block = kmalloc(block_size);
    if (indirect_block == 0) {
        EXT2_ERROR("Failed to allocate memory for indirect block");
        return EXT2_READ_FAILED;
    }

    if (ext2_read_block(partition, block, (uint8_t*)indirect_block) == EXT2_READ_FAILED) {
        EXT2_ERROR("Failed to read indirect block");
        kfree(indirect_block);
        return EXT2_READ_FAILED;
    }

    uint32_t blocks_read = 0;
    for (uint32_t i = 0; i < block_size / 4; i++) {
        if (indirect_block[i] == 0 || indirect_block[i] == INODE_BLOCK_END) {
            break;
        }

        uint32_t read = ext2_load_indirect_block_list(partition, block_buffer + blocks_read, indirect_block[i]);
        if (read == EXT2_READ_FAILED) {
            EXT2_ERROR("Failed to read indirect block");
            kfree(indirect_block);
            return EXT2_READ_FAILED;
        }

        blocks_read += read;
    }

    kfree(indirect_block);
    return blocks_read;
}

uint32_t ext2_load_triple_indirect_block_list(struct ext2_partition* partition, uint32_t * block_buffer, uint32_t block) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    uint32_t * indirect_block = kmalloc(block_size);
    if (indirect_block == 0) {
        EXT2_ERROR("Failed to allocate memory for indirect block");
        return EXT2_READ_FAILED;
    }

    if (ext2_read_block(partition, block, (uint8_t*)indirect_block) == EXT2_READ_FAILED) {
        EXT2_ERROR("Failed to read indirect block");
        kfree(indirect_block);
        return EXT2_READ_FAILED;
    }

    uint32_t blocks_read = 0;
    for (uint32_t i = 0; i < block_size / 4; i++) {
        if (indirect_block[i] == 0 || indirect_block[i] == INODE_BLOCK_END) {
            break;
        }

        uint32_t read = ext2_load_double_indirect_block_list(partition, block_buffer + blocks_read, indirect_block[i]);
        if (read == EXT2_READ_FAILED) {
            EXT2_ERROR("Failed to read indirect block");
            kfree(indirect_block);
            return EXT2_READ_FAILED;
        }

        blocks_read += read;
    }

    kfree(indirect_block);
    return blocks_read;
}

uint32_t* ext2_load_block_list(struct ext2_partition* partition, uint32_t inode_number) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    
    struct ext2_inode_descriptor_generic * inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_number);
    if (inode == 0) {
        EXT2_WARN("Failed to read root inode");
        return 0;
    }
    
    uint32_t block_number = DIVIDE_ROUNDED_UP(inode->i_size, block_size);

    uint32_t * block_list = kmalloc(block_number * 4);
    if (block_list == 0) {
        EXT2_ERROR("Failed to allocate memory for block list");
        return 0;
    }

    uint32_t block_index = 0;
    
    for (uint32_t i = 0; i < 12; i++) {
        if (block_index >= block_number) {
            break;
        }

        if (inode->i_block[i] > 0) {
            block_list[block_index] = inode->i_block[i];
            block_index++;
        }
    }

    if (block_index < block_number) {
        EXT2_DEBUG("Reading indirect block list");
        uint32_t read = ext2_load_indirect_block_list(partition, block_list + block_index, inode->i_block[12]);
        if (read == EXT2_READ_FAILED) {
            EXT2_ERROR("Failed to read indirect block");
            kfree(block_list);
            return 0;
        }

        block_index += read;
    }

    if (block_index < block_number) {
        EXT2_DEBUG("[%d/%d] Reading double indirect block list", block_index, block_number);
        uint32_t read = ext2_load_double_indirect_block_list(partition, block_list + block_index, inode->i_block[13]);
        if (read == EXT2_READ_FAILED) {
            EXT2_ERROR("Failed to read indirect block");
            kfree(block_list);
            return 0;
        }

        block_index += read;
    }

    if (block_index < block_number) {
        EXT2_DEBUG("Reading triple block list");
        uint32_t read = ext2_load_triple_indirect_block_list(partition, block_list + block_index, inode->i_block[14]);
        if (read == EXT2_READ_FAILED) {
            EXT2_ERROR("Failed to read indirect block");
            kfree(block_list);
            return 0;
        }

        block_index += read;
    }

    if (block_index != block_number) {
        EXT2_ERROR("Block list size mismatch! Expected %d, got %d", block_number, block_index);
        kfree(block_list);
        return 0;
    } else {
        EXT2_DEBUG("Block list size matches inode size");
    }

    return block_list;

}

int64_t ext2_delete_file_blocks(struct ext2_partition* partition, uint32_t inode_number) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    struct ext2_inode_descriptor_generic * inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_number);
    if (inode == 0) {
        EXT2_ERROR("Failed to read inode %d", inode_number);
        return 1;
    }

    uint32_t * block_list = ext2_load_block_list(partition, inode_number);
    if (block_list == 0) {
        EXT2_ERROR("Failed to load block list for inode %d", inode_number);
        return 1;
    }

    uint32_t block_number = DIVIDE_ROUNDED_UP(inode->i_size, block_size);
    
    EXT2_DEBUG("Deallocating %d blocks for inode %d", block_number, inode_number);

    if (ext2_deallocate_blocks(partition, block_list, block_number) != block_number) {
        EXT2_ERROR("Failed to deallocate blocks for inode %d", inode_number);
        kfree(block_list);
        return 1;
    }

    kfree(block_list);
    return 0;
}

uint8_t ext2_delete_n_blocks(struct ext2_partition* partition, uint32_t inode_number, uint32_t blocks_to_remove) {
    struct ext2_inode_descriptor_generic * inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_number);
    if (inode == 0) {
        EXT2_ERROR("Failed to read inode %d", inode_number);
        return 1;
    }

    uint32_t * block_list = ext2_load_block_list(partition, inode_number);
    if (block_list == 0) {
        EXT2_ERROR("Failed to load block list for inode %d", inode_number);
        return 1;
    }
    
    EXT2_DEBUG("Deallocating %d blocks for inode %d", blocks_to_remove, inode_number);
    if (ext2_deallocate_blocks(partition, block_list, blocks_to_remove) != blocks_to_remove) {
        EXT2_ERROR("Failed to deallocate blocks for inode %d", inode_number);
        kfree(block_list);
        return 1;
    }

    kfree(block_list);
    return 0;
}

uint8_t ext2_delete_inode(struct ext2_partition* partition, uint32_t inode_number) {
    struct ext2_inode_descriptor * full_inode = ext2_read_inode(partition, inode_number);
    if (full_inode == 0) {
        EXT2_ERROR("Failed to read inode %d", inode_number);
        return 1;
    }

    struct ext2_inode_descriptor_generic * inode = &(full_inode->id);
    uint32_t original_mode = inode->i_mode;
    
    struct ext2_superblock * superblock = (struct ext2_superblock*)partition->sb;
    uint32_t inode_group = (inode_number - 1 ) / superblock->s_inodes_per_group;
    uint32_t inode_index = (inode_number - 1) % superblock->s_inodes_per_group;

    inode->i_dtime = ext2_get_current_epoch();
    inode->i_links_count = 0;
    inode->i_size = 0;
    inode->i_mode = 0;
    inode->i_sectors = 0;

    if (ext2_write_inode(partition, inode_number, full_inode)) {
        EXT2_ERROR("Failed to delete inode %d", inode_number);
        return 1;
    }

    //Delete inode bitmap

    uint8_t * inode_bitmap_buffer = ext2_read_inode_bitmap(partition, inode_number);
    if (inode_bitmap_buffer == 0) {
        EXT2_ERROR("Failed to read inode bitmap %d", inode_number);
        return 1;
    }

    inode_bitmap_buffer[inode_index] = 0;
    if (ext2_write_inode_bitmap(partition, inode_number, inode_bitmap_buffer)) {
        EXT2_ERROR("Failed to delete inode bitmap %d", inode_number);
        return 1;
    }
    kfree(inode_bitmap_buffer);

    //Update group descriptor
    partition->gd[inode_group].bg_free_inodes_count++;
    if (original_mode & INODE_TYPE_DIR) {
        partition->gd[inode_group].bg_used_dirs_count--;
    }
    
    //Update superblock
    superblock->s_free_inodes_count++;
    
    ext2_flush_required(partition);
    return 0;
}

void ext2_dump_inode(struct ext2_inode_descriptor_generic * inode) {
    kprintf("i_mode: %x i_uid: %x i_size: %x\n", inode->i_mode, inode->i_uid, inode->i_size);
    kprintf("i_atime: %x i_ctime: %x i_mtime: %x i_dtime: %x\n", inode->i_atime, inode->i_ctime, inode->i_mtime, inode->i_dtime);
    kprintf("i_gid: %x i_links_count: %x i_sectors: %x i_flags: %x i_osd1: %x\n", inode->i_gid, inode->i_links_count, inode->i_sectors, inode->i_flags, inode->i_osd1);
    kprintf("i_generation: %x i_file_acl: %x i_dir_acl: %x i_faddr: %x\n", inode->i_generation, inode->i_file_acl, inode->i_dir_acl, inode->i_faddr);

    for (uint32_t i = 0; i < 15; i++) {
        kprintf("i_block[%d]: %x", i, inode->i_block[i]);
    }
    kprintf("\n");
}

uint8_t ext2_dentry_walk_inodes_cb(struct ext2_partition* partition, uint32_t inode_index) {
    struct ext2_inode_descriptor_generic * inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_index);
    if (inode == 0) {
        EXT2_ERROR("Failed to read inode %d, but we dont care", inode_index);
        return 0;
    }

    ext2_dump_inode(inode);
    return 0;
}

void ext2_dump_all_inodes(struct ext2_partition* partition, const char* root_path) {

    struct ext2_directory_entry * entries;
    uint32_t entry_count = ext2_get_all_dirs(partition, root_path, &entries);

    for (uint32_t i = 0; i < entry_count; i++) {
        struct ext2_directory_entry * entry = &entries[i];
        if (entry->inode == 0) {
            continue;
        }

        kprintf("Entry %d: %s%s, inode %d\n", i, root_path, entry->name, entry->inode);
        ext2_dentry_walk_inodes_cb(partition, entry->inode);

        if (entry->file_type == EXT2_DIR_TYPE_DIRECTORY) {
            if (strncmp(entry->name, ".", entry->name_len) == 0 || strncmp(entry->name, "..", entry->name_len) == 0) {
                continue;
            }
            
            char * new_path = kmalloc(strlen(root_path) + strlen(entry->name) + 2);
            if (new_path == 0) {
                EXT2_ERROR("Failed to allocate memory for path");
                return;
            }
            
            strncpy(new_path, root_path, strlen(root_path) + 1);
            strcat(new_path, "/");
            strcat(new_path, entry->name);
            ext2_dump_all_inodes(partition, new_path);
            kfree(new_path);
        }
    }
    
    kfree(entries);
}