//Supress -Waddress-of-packed-member warning
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#pragma GCC diagnostic ignored "-Wvariadic-macros"

#include "ext2_block.h"

#include "ext2_inode.h"

#include "ext2_util.h"
#include "ext2_integrity.h"

#include <omen/libraries/std/stdint.h>
#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <disk/disk_interface.h>


int64_t ext2_read_block(struct ext2_partition* partition, uint32_t block, uint8_t * destination_buffer) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    uint32_t block_sectors = DIVIDE_ROUNDED_UP(block_size, partition->sector_size);
    uint32_t block_lba = (block * block_size) / partition->sector_size;

    if (block > 0) {
        if (!read_disk(partition->disk, destination_buffer, partition->lba + block_lba, block_sectors*partition->sector_size)) {
            EXT2_ERROR("Failed to read block %d", block);
            return EXT2_READ_FAILED;
        }
    } else {
        EXT2_ERROR("Attempted to read invalid block %d", block);
        return 0;
    }

    return 1;
}

int64_t ext2_read_direct_blocks(struct ext2_partition* partition, uint32_t * blocks, uint32_t max, uint8_t * destination_buffer, uint64_t count, uint64_t * skip_count) {
    EXT2_DEBUG("Reading direct blocks blocks: %llx, max: %d, count: %d, skip: %d", blocks, max, count, *skip_count);
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    uint64_t blocks_read = 0;
    
    for (uint32_t i = 0; i < max; i++) {
        if (*skip_count > 0) {
            (*skip_count)--;
            continue;
        }

        int64_t read_result = ext2_read_block(partition, blocks[i], destination_buffer);

        if (read_result == EXT2_READ_FAILED) {
            EXT2_ERROR("Failed to read block %d", blocks[i]);
            return EXT2_READ_FAILED;
        } else if (read_result == 0) {
            EXT2_DEBUG("Read 0 blocks");
            return blocks_read;
        }

        destination_buffer += block_size;
        blocks_read += read_result;
        
        if (blocks_read == count) {
            break;
        }

    }
    EXT2_DEBUG("Read %d blocks", blocks_read);
    return blocks_read;
}

int64_t ext2_read_indirect_blocks(struct ext2_partition* partition, uint32_t * blocks, uint32_t max, uint8_t *destination_buffer, uint64_t count, uint64_t * skip) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    uint32_t entries_per_block = block_size / 4; //TODO: Abstract for n case
    uint64_t blocks_read = 0;

    for (uint32_t i = 0; i < max; i++) {
        uint32_t * indirect_block = (uint32_t*)ext2_buffer_for_size(block_size, block_size);
        if (blocks[i] == 0) EXT2_WARN("Single indirect block is 0");

        if (ext2_read_block(partition, blocks[i], (uint8_t*)indirect_block) <= 0) {
            EXT2_ERROR("Failed to read indirect block");
            kfree(indirect_block);
            return EXT2_READ_FAILED;
        }

        int64_t read_result = ext2_read_direct_blocks(partition, indirect_block, entries_per_block, destination_buffer, count - blocks_read , skip);
        kfree(indirect_block);
        if (read_result == EXT2_READ_FAILED || (read_result == 0 && *skip <= 0)) return read_result;

        destination_buffer += (read_result * block_size);
        blocks_read += read_result;

        if (blocks_read == count) {
            break;
        }
    }

    return blocks_read;
}

int64_t ext2_read_double_indirect_blocks(struct ext2_partition* partition, uint32_t * blocks, uint32_t max, uint8_t *destination_buffer, uint64_t count, uint64_t * skip) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    uint32_t entries_per_block = block_size / 4;
    uint32_t blocks_read = 0;

    for (uint32_t i = 0; i < max; i++) {
        uint32_t * double_indirect_block = (uint32_t*)ext2_buffer_for_size(block_size, block_size);
        if (blocks[i] == 0) EXT2_WARN("Double indirect block is 0");
        if (ext2_read_block(partition, blocks[i], (uint8_t*)double_indirect_block) <= 0) {
            EXT2_ERROR("Failed to read double indirect block");
            kfree(double_indirect_block);
            return EXT2_READ_FAILED;
        }
        int64_t read_result = ext2_read_indirect_blocks(partition, double_indirect_block, entries_per_block, destination_buffer, count - blocks_read, skip);
        kfree(double_indirect_block);

        if (read_result == EXT2_READ_FAILED || (read_result == 0 && *skip <= 0))  return read_result;


        destination_buffer += (read_result * block_size);
        blocks_read += read_result;

        if (blocks_read == count) {
            break;
        }
    }

    return blocks_read;
}

int64_t ext2_read_triple_indirect_blocks(struct ext2_partition* partition, uint32_t * blocks, uint32_t max, uint8_t *destination_buffer, uint64_t count, uint64_t* skip) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    uint32_t entries_per_block = block_size / 4;
    uint32_t blocks_read = 0;

    for (uint32_t i = 0; i < max; i++) {
        uint32_t * triple_indirect_block = (uint32_t*)ext2_buffer_for_size(block_size, block_size);
        if (blocks[i] == 0) EXT2_WARN("Triple indirect block is 0");
        if (ext2_read_block(partition, blocks[i], (uint8_t*)triple_indirect_block) <= 0) {
            EXT2_ERROR("Failed to read triple indirect block");
            kfree(triple_indirect_block);
            return EXT2_READ_FAILED;
        }

        int64_t read_result = ext2_read_double_indirect_blocks(partition, triple_indirect_block, entries_per_block, destination_buffer, count - blocks_read, skip);
        kfree(triple_indirect_block);

        if (read_result == EXT2_READ_FAILED || (read_result == 0 && *skip <= 0))  return read_result;

        destination_buffer += (read_result * block_size);
        blocks_read += read_result;

        if (blocks_read == count) {
            break;
        }
    }

    return blocks_read;
}


int64_t ext2_write_block(struct ext2_partition* partition, uint32_t block, uint8_t * source_buffer) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    uint32_t block_sectors = DIVIDE_ROUNDED_UP(block_size, partition->sector_size);
    uint32_t block_lba = (block * block_size) / partition->sector_size;

    if (block) {
        if (!write_disk(partition->disk, source_buffer, partition->lba + block_lba, block_sectors)) {
            EXT2_ERROR("Failed to write block %d", block);
            return EXT2_WRITE_FAILED;
        }
    } else {
        EXT2_ERROR("Attempted to write block 0");
        return 0;
    }

    return 1;
}

int64_t ext2_write_direct_blocks(struct ext2_partition* partition, uint32_t * blocks, uint32_t max, uint8_t * source_buffer, uint64_t count, uint64_t * skip) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    uint64_t blocks_written = 0;

    for (uint32_t i = 0; i < max; i++) {
        if (blocks[i] == 0) EXT2_WARN("Direct block is 0");
        if (*skip > 0) {
            *skip -= 1;
            continue;
        }
        int64_t write_result = ext2_write_block(partition, blocks[i], source_buffer);

        if (write_result == EXT2_WRITE_FAILED) {
            return EXT2_WRITE_FAILED;
        } else if (write_result == 0) {
            return blocks_written;
        }

        source_buffer += block_size;
        blocks_written += write_result;
        
        if (blocks_written == count) {
            break;
        }

    }

    return blocks_written;
}

int64_t ext2_write_indirect_blocks(struct ext2_partition* partition, uint32_t * blocks, uint32_t max, uint8_t *source_buffer, uint64_t count, uint64_t * skip) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    uint32_t entries_per_block = block_size / 4; //TODO: Abstract for n case
    uint64_t blocks_written = 0;

    for (uint32_t i = 0; i < max; i++) {
        uint32_t * indirect_block = (uint32_t*)ext2_buffer_for_size(block_size, block_size);
        if (blocks[i] == 0) EXT2_WARN("Single indirect block is 0");

        if (ext2_read_block(partition, blocks[i], (uint8_t*)indirect_block) <= 0) {
            EXT2_ERROR("Failed to read indirect block");
            kfree(indirect_block);
            return EXT2_WRITE_FAILED;
        }

        int64_t write_result = ext2_write_direct_blocks(partition, indirect_block, entries_per_block, source_buffer, count - blocks_written, skip);
        kfree(indirect_block);
        if (write_result == EXT2_WRITE_FAILED || write_result == 0) return write_result;

        source_buffer += (write_result * block_size);
        blocks_written += write_result;

        if (blocks_written == count) {
            break;
        }
    }

    return blocks_written;
}

int64_t ext2_write_double_indirect_blocks(struct ext2_partition* partition, uint32_t * blocks, uint32_t max, uint8_t *source_buffer, uint64_t count, uint64_t * skip) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    uint32_t entries_per_block = block_size / 4;
    uint32_t blocks_written = 0;

    for (uint32_t i = 0; i < max; i++) {
        uint32_t * double_indirect_block = (uint32_t*)ext2_buffer_for_size(block_size, block_size);
        if (blocks[i] == 0) EXT2_WARN("Double indirect block is 0");
        if (ext2_read_block(partition, blocks[i], (uint8_t*)double_indirect_block) <= 0) {
            EXT2_ERROR("Failed to read double indirect block");
            kfree(double_indirect_block);
            return EXT2_WRITE_FAILED;
        }
        int64_t write_result = ext2_write_indirect_blocks(partition, double_indirect_block, entries_per_block, source_buffer, count - blocks_written, skip);
        kfree(double_indirect_block);

        if (write_result == EXT2_WRITE_FAILED || write_result == 0) return write_result;


        source_buffer += (write_result * block_size);
        blocks_written += write_result;

        if (blocks_written == count) {
            break;
        }
    }

    return blocks_written;
}

int64_t ext2_write_triple_indirect_blocks(struct ext2_partition* partition, uint32_t * blocks, uint32_t max, uint8_t *source_buffer, uint64_t count, uint64_t * skip) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    uint32_t entries_per_block = block_size / 4;
    uint32_t blocks_written = 0;

    for (uint32_t i = 0; i < max; i++) {
        uint32_t * triple_indirect_block = (uint32_t*)ext2_buffer_for_size(block_size, block_size);
        if (blocks[i] == 0) EXT2_WARN("Triple indirect block is 0");
        if (ext2_read_block(partition, blocks[i], (uint8_t*)triple_indirect_block) <= 0) {
            EXT2_ERROR("Failed to read triple indirect block (for write)");
            kfree(triple_indirect_block);
            return EXT2_WRITE_FAILED;
        }

        int64_t write_result = ext2_write_double_indirect_blocks(partition, triple_indirect_block, entries_per_block, source_buffer, count - blocks_written, skip);
        kfree(triple_indirect_block);

        if (write_result == EXT2_WRITE_FAILED || write_result == 0) return write_result;

        source_buffer += (write_result * block_size);
        blocks_written += write_result;

        if (blocks_written == count) {
            break;
        }
    }

    return blocks_written;
}



//TODO: Optimize this so you start searching from the last block allocated
uint32_t ext2_allocate_block(struct ext2_partition* partition) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    for (uint32_t i = 0; i < partition->group_number; i++) {
        struct ext2_block_group_descriptor * block_group = &partition->gd[i];
        if (!block_group) {
            EXT2_ERROR("Failed to read block group descriptor");
            return 0;
        }

        if (block_group->bg_free_blocks_count == 0) {
            EXT2_ERROR("Block group %d is full", i);
            continue;
        }

        uint32_t * block_bitmap = (uint32_t *)kmalloc(block_size);
        if (!block_bitmap) {
            EXT2_ERROR("Failed to allocate block bitmap");
            return 0;
        }

        if (!ext2_read_block(partition, block_group->bg_block_bitmap, (uint8_t*)block_bitmap)) {
            EXT2_ERROR("Failed to read block bitmap");
            kfree(block_bitmap);
            return 0;
        }

        for (uint32_t j = 0; j < block_size / sizeof(uint32_t); j++) {
            if (block_bitmap[j] == 0xFFFFFFFF) continue;
            for (uint32_t k = 0; k < 32; k++) {
                if (!(block_bitmap[j] & (1 << k))) {
                    block_bitmap[j] |= (1 << k);
                    block_group->bg_free_blocks_count--;
                    ((struct ext2_superblock*)partition->sb)->s_free_blocks_count--;
                    if (ext2_write_block(partition, block_group->bg_block_bitmap, (uint8_t*)block_bitmap)  <= 0) {
                        EXT2_ERROR("Failed to write block bitmap");
                        kfree(block_bitmap);
                        //Undo the changes
                        block_group->bg_free_blocks_count++;
                        ((struct ext2_superblock*)partition->sb)->s_free_blocks_count++;
                        return 0;
                    }

                    ext2_flush_required(partition);
                    uint32_t block = j * 32 + k + 1;
                    block += i * ((struct ext2_superblock*)partition->sb)->s_blocks_per_group;
                    kfree(block_bitmap);
                    return block;
                }
            }
        }

        kfree(block_bitmap);
    }

    EXT2_ERROR("No free blocks");
    return 0;
}

uint32_t ext2_deallocate_blocks(struct ext2_partition* partition, uint32_t *blocks, uint32_t block_number) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    uint32_t blocks_deallocated = 0;
    uint8_t * block_bitmap = kmalloc(block_size);
    if (!block_bitmap) {
        EXT2_ERROR("Failed to allocate block bitmap");
        return 0;
    }

    //Start from the last block of the file
    for (int i = block_number - 1; i >= 0; i--) {
        if (!ext2_deallocate_block(partition, blocks[i])) {
            EXT2_ERROR("Failed to deallocate block %d", blocks[i]);
            kfree(block_bitmap);
            return 0;
        }

        blocks_deallocated++;
    }

    kfree(block_bitmap);
    ext2_flush_required(partition);
    EXT2_DEBUG("Deallocated %d blocks", blocks_deallocated);
    return blocks_deallocated;
}

uint32_t ext2_deallocate_block(struct ext2_partition* partition, uint32_t block) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    uint32_t block_group = (block - 1) / ((struct ext2_superblock*)partition->sb)->s_blocks_per_group;
    uint32_t block_index = (block - 1) % ((struct ext2_superblock*)partition->sb)->s_blocks_per_group;
    uint32_t block_bitmap_index = block_index / 32;
    uint32_t block_bitmap_offset = block_index % 32;

    struct ext2_block_group_descriptor * block_group_descriptor = &partition->gd[block_group];
    if (!block_group_descriptor) {
        EXT2_ERROR("Failed to read block group descriptor");
        return 0;
    }

    uint32_t * block_bitmap = (uint32_t *)kmalloc(block_size);
    if (!block_bitmap) {
        EXT2_ERROR("Failed to allocate block bitmap");
        return 0;
    }
    
    if (!ext2_read_block(partition, block_group_descriptor->bg_block_bitmap, (uint8_t*)block_bitmap)) {
        EXT2_ERROR("Failed to read block bitmap");
        kfree(block_bitmap);
        return 0;
    }

    if (block_bitmap[block_bitmap_index] & (1 << block_bitmap_offset)) {
        block_bitmap[block_bitmap_index] &= ~(1 << block_bitmap_offset);
        block_group_descriptor->bg_free_blocks_count++;
        ((struct ext2_superblock*)partition->sb)->s_free_blocks_count++;
        if (ext2_write_block(partition, block_group_descriptor->bg_block_bitmap, (uint8_t*)block_bitmap)  <= 0) {
            EXT2_ERROR("Failed to write block bitmap");
            kfree(block_bitmap);
            //Undo the changes
            block_group_descriptor->bg_free_blocks_count--;
            ((struct ext2_superblock*)partition->sb)->s_free_blocks_count--;
            return 0;
        }

        ext2_flush_required(partition);
        kfree(block_bitmap);
        return 1;
    }

    kfree(block_bitmap);
    return 0;
}

uint32_t ext2_allocate_indirect_block(struct ext2_partition* partition, uint32_t * target_block, uint32_t blocks_to_allocate) {
        EXT2_DEBUG("Allocating block group");
        uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
        if (!target_block) {
            EXT2_ERROR("Invalid target block");
            return 0;
        }
        *target_block = ext2_allocate_block(partition);
        if (!*target_block) {
            EXT2_ERROR("Failed to allocate block");
            return 0;
        }

        uint32_t * block_buffer = (uint32_t *)kmalloc(block_size);
        if (!block_buffer) {
            EXT2_ERROR("Failed to allocate memory for block buffer");
            return 0;
        }

        uint32_t blocks_written = 0;
        for (uint32_t j = 0; j < block_size / sizeof(uint32_t); j++) {
            if (blocks_written == blocks_to_allocate) break;
            block_buffer[j] = ext2_allocate_block(partition);
            if (!block_buffer[j]) {
                EXT2_ERROR("Failed to allocate block");
                kfree(block_buffer);
                return 0;
            }
            blocks_written++;
        }

        if (ext2_write_block(partition, *target_block, (uint8_t*)block_buffer) <= 0) {
            EXT2_ERROR("Failed to write block");
            kfree(block_buffer);
            return 0;
        }

        kfree(block_buffer);
        return blocks_written;
}

uint32_t ext2_allocate_double_indirect_block(struct ext2_partition* partition, uint32_t * target_block, uint32_t blocks_to_allocate) {
    EXT2_DEBUG("Allocating double indirect block");
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    if (!target_block) {
        EXT2_ERROR("Invalid target block");
        return 0;
    }
    *target_block = ext2_allocate_block(partition);
    if (!*target_block) {
        EXT2_ERROR("Failed to allocate block");
        return 0;
    }

    uint32_t * block_buffer = (uint32_t *)kmalloc(block_size);
    if (!block_buffer) {
        EXT2_ERROR("Failed to allocate memory for block buffer");
        return 0;
    }

    uint32_t blocks_written = 0;
    for (uint32_t j = 0; j < block_size / sizeof(uint32_t); j++) {
        if (blocks_written == blocks_to_allocate) break;
        blocks_written += ext2_allocate_indirect_block(partition, &block_buffer[j], blocks_to_allocate - blocks_written);
    }

    if (ext2_write_block(partition, *target_block, (uint8_t*)block_buffer) <= 0) {
        EXT2_ERROR("Failed to write block");
        kfree(block_buffer);
        return 0;
    }

    kfree(block_buffer);
    return blocks_written;
}

uint32_t ext2_allocate_triple_indirect_block(struct ext2_partition* partition, uint32_t * target_block, uint32_t blocks_to_allocate) {
    EXT2_DEBUG("Allocating triple indirect block");
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    if (!target_block) {
        EXT2_ERROR("Invalid target block");
        return 0;
    }
    *target_block = ext2_allocate_block(partition);
    if (!*target_block) {
        EXT2_ERROR("Failed to allocate block");
        return 0;
    }

    uint32_t * block_buffer = (uint32_t *)kmalloc(block_size);
    if (!block_buffer) {
        EXT2_ERROR("Failed to allocate memory for block buffer");
        return 0;
    }

    uint32_t blocks_written = 0;
    for (uint32_t j = 0; j < block_size / sizeof(uint32_t); j++) {
        if (blocks_written == blocks_to_allocate) break;
        blocks_written += ext2_allocate_double_indirect_block(partition, &block_buffer[j], blocks_to_allocate - blocks_written);
    }

    if (ext2_write_block(partition, *target_block, (uint8_t*)block_buffer) <= 0) {
        EXT2_ERROR("Failed to write block");
        kfree(block_buffer);
        return 0;
    }

    kfree(block_buffer);
    return blocks_written;
}


uint8_t ext2_allocate_blocks(struct ext2_partition* partition, struct ext2_inode_descriptor_generic * inode, uint32_t blocks_to_allocate) {
    uint32_t blocks_allocated = 0;
    for (uint32_t i = 0; i < 12; i++) {
        if (blocks_allocated == blocks_to_allocate) break;
        if (!inode->i_block[i]) {
            inode->i_block[i] = ext2_allocate_block(partition);
            if (!inode->i_block[i]) {
                EXT2_ERROR("Failed to allocate block");
                return 1;
            }
            blocks_allocated++;
        }
    }

    if (blocks_allocated == blocks_to_allocate) {
        EXT2_DEBUG("Allocated all blocks");
        return 0;
    }

    blocks_allocated += ext2_allocate_indirect_block(partition, &inode->i_block[12], blocks_to_allocate - blocks_allocated);

    if (blocks_allocated == blocks_to_allocate) {
        EXT2_DEBUG("Allocated all blocks");
        return 0;
    }

    blocks_allocated += ext2_allocate_double_indirect_block(partition, &inode->i_block[13], blocks_to_allocate - blocks_allocated);

    if (blocks_allocated == blocks_to_allocate) {
        EXT2_DEBUG("Allocated all blocks");
        return 0;
    }

    blocks_allocated += ext2_allocate_triple_indirect_block(partition, &inode->i_block[14], blocks_to_allocate - blocks_allocated);

    if (blocks_allocated == blocks_to_allocate) {
        EXT2_DEBUG("Allocated all blocks");
        return 0;
    }

    EXT2_ERROR("Failed to allocate all blocks");
    return 1;
}

int64_t ext2_read_inode_blocks(struct ext2_partition* partition, uint32_t inode_number, uint8_t * destination_buffer, uint64_t count, uint64_t blocks_skip) {
    if (count == 0) return 0;
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    struct ext2_inode_descriptor_generic * inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_number);
    if (inode == 0) return EXT2_READ_FAILED;

    uint32_t blocks_read = 0;
    int64_t read_result = 0;
    EXT2_DEBUG("First file block: %d", inode->i_block[0]);
    
    read_result = ext2_read_direct_blocks(partition, inode->i_block, 12, destination_buffer, count, &blocks_skip);
    if (read_result == EXT2_READ_FAILED) return read_result;
    blocks_read += read_result;
    destination_buffer += read_result * block_size;
    EXT2_DEBUG("Basic read %d blocks, max: %ld", blocks_read, count);
    if (blocks_read >= count) return blocks_read;

    EXT2_DEBUG("Reading indirect block");
    read_result = ext2_read_indirect_blocks(partition, &(inode->i_block[12]), 1, destination_buffer, count - blocks_read, &blocks_skip);
    if (read_result == EXT2_READ_FAILED) return read_result;
    blocks_read += read_result;
    destination_buffer += read_result * block_size;
    EXT2_DEBUG("Indirect read %d blocks, max: %ld", blocks_read, count);
    if (blocks_read >= count) return blocks_read;

    EXT2_DEBUG("Reading double indirect block");
    read_result = ext2_read_double_indirect_blocks(partition, &(inode->i_block[13]), 1, destination_buffer, count - blocks_read, &blocks_skip);
    if (read_result == EXT2_READ_FAILED) return read_result;
    blocks_read += read_result;
    destination_buffer += read_result * block_size;
    EXT2_DEBUG("Double indirect read %d blocks, max: %ld", blocks_read, count);
    if (blocks_read >= count) return blocks_read;

    EXT2_DEBUG("Reading triple indirect block");
    read_result = ext2_read_triple_indirect_blocks(partition, &(inode->i_block[14]), 1, destination_buffer, count - blocks_read, &blocks_skip);
    if (read_result == EXT2_READ_FAILED) return read_result;
    blocks_read += read_result;
    destination_buffer += read_result * block_size;
    EXT2_DEBUG("Triple indirect read %d blocks, max: %ld", blocks_read, count);
    if (blocks_read >= count) return blocks_read;

    EXT2_ERROR("File is too big for block size: %d", block_size);
    return EXT2_READ_FAILED;
}

int64_t ext2_read_inode_bytes(struct ext2_partition* partition, uint32_t inode_number, uint8_t * destination_buffer, uint64_t count, uint64_t skip) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    //struct ext2_inode_descriptor_generic * inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_number);
    //if (inode == 0) return EXT2_READ_FAILED;
    //uint32_t file_size = inode->i_size;
    uint64_t blocks = DIVIDE_ROUNDED_UP(count, block_size);
    uint64_t excess_bytes = (blocks * block_size) - count;
    uint64_t skip_blocks = skip / block_size;
    uint64_t skip_bytes = skip % block_size;

    uint64_t read_bytes = 0;
    EXT2_DEBUG("reading %ld bytes from ino: %d to %p [skip_blocks: %ld, skip_bytes: %ld, excess_bytes: %ld]", count, inode_number, (void*)destination_buffer, skip_blocks, skip_bytes, excess_bytes);

    if (skip_bytes) {
        uint8_t * temp_buffer = (uint8_t *)kmalloc(block_size);
        int64_t skip_read_result = ext2_read_inode_blocks(partition, inode_number, temp_buffer, 1, skip_blocks);
        if (skip_read_result != 1) {
            EXT2_ERROR("Failed to read skip bytes");
            return EXT2_READ_FAILED;
        }

        memcpy(destination_buffer, temp_buffer + skip_bytes, block_size - skip_bytes);
        kfree(temp_buffer);
        destination_buffer += block_size - skip_bytes;
        skip_blocks++;
        read_bytes += block_size - skip_bytes;
    }

    int64_t remaining_bytes = count - read_bytes;
    if (remaining_bytes > 0) {
        int64_t result = ext2_read_inode_blocks(partition, inode_number, destination_buffer, blocks - 1, skip_blocks);
        if (result == EXT2_READ_FAILED) return EXT2_READ_FAILED;

        if (result < 0) {
            EXT2_ERROR("Failed to read file");
            return EXT2_READ_FAILED;
        }

        read_bytes += result * block_size;
        skip_blocks += result;
        destination_buffer += result * block_size;
        remaining_bytes = count - read_bytes;
    }

    if (remaining_bytes > 0) {
        uint8_t * temp_buffer = (uint8_t *)kmalloc(block_size);
        int64_t remaining_read_result = ext2_read_inode_blocks(partition, inode_number, temp_buffer, 1, skip_blocks);
        if (remaining_read_result != 1) {
            EXT2_ERROR("Failed to read remaining bytes");
            return EXT2_READ_FAILED;
        }
        memcpy(destination_buffer, temp_buffer, remaining_bytes);
        kfree(temp_buffer);
        read_bytes += remaining_bytes;
    }

    EXT2_DEBUG("Full read: %ld bytes", read_bytes);
    return read_bytes;
}

int64_t ext2_write_inode_blocks(struct ext2_partition* partition, uint32_t inode_number, uint8_t * source_buffer, uint64_t count, uint64_t blocks_skip) {
    if (count == 0) return 0;  
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    struct ext2_inode_descriptor_generic * inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_number);
    if (inode == 0) return EXT2_WRITE_FAILED;

    uint32_t blocks_written = 0;
    int64_t write_result = 0;

    EXT2_DEBUG("Writing basic blocks %d", inode->i_block[0]);
    write_result = ext2_write_direct_blocks(partition, inode->i_block, 12, source_buffer, count, &blocks_skip);
    if (write_result == EXT2_WRITE_FAILED || write_result == 0) return write_result;
    blocks_written += write_result;
    source_buffer += write_result * block_size;
    EXT2_DEBUG("Basic write %d blocks, max: %ld", blocks_written, count);
    if (blocks_written >= count) return blocks_written;

    EXT2_DEBUG("Writing indirect block");
    write_result = ext2_write_indirect_blocks(partition, &(inode->i_block[12]), 1, source_buffer, count - blocks_written, &blocks_skip);
    if (write_result == EXT2_WRITE_FAILED || write_result == 0) return write_result;
    blocks_written += write_result;
    source_buffer += write_result * block_size;
    EXT2_DEBUG("Indirect write %d blocks, max: %ld", blocks_written, count);
    if (blocks_written >= count) return blocks_written;

    EXT2_DEBUG("Writing double indirect block");
    write_result = ext2_write_double_indirect_blocks(partition, &(inode->i_block[13]), 1, source_buffer, count - blocks_written, &blocks_skip);
    if (write_result == EXT2_WRITE_FAILED || write_result == 0) return write_result;
    blocks_written += write_result;
    source_buffer += write_result * block_size;
    EXT2_DEBUG("Double indirect write %d blocks, max: %ld", blocks_written, count);
    if (blocks_written >= count) return blocks_written;

    EXT2_DEBUG("Writing triple indirect block");
    write_result = ext2_write_triple_indirect_blocks(partition, &(inode->i_block[14]), 1, source_buffer, count - blocks_written, &blocks_skip);
    if (write_result == EXT2_WRITE_FAILED || write_result == 0) return write_result;
    blocks_written += write_result;
    source_buffer += write_result * block_size;
    EXT2_DEBUG("Triple indirect write  %d blocks, max: %ld", blocks_written, count);
    if (blocks_written >= count) return blocks_written;

    EXT2_ERROR("File is too big for block size: %d", block_size);
    return EXT2_WRITE_FAILED;
}

int64_t ext2_write_inode_bytes(struct ext2_partition* partition, uint32_t inode_number, uint8_t * source_buffer, uint64_t count, uint64_t skip) {
    uint32_t block_size = 1024 << (((struct ext2_superblock*)partition->sb)->s_log_block_size);
    struct ext2_inode_descriptor_generic * inode = (struct ext2_inode_descriptor_generic *)ext2_read_inode(partition, inode_number);
    if (inode == 0) return EXT2_WRITE_FAILED;
    uint32_t inode_size = inode->i_size;
    uint64_t blocks = DIVIDE_ROUNDED_UP(count, block_size);
    uint64_t excess_bytes = (blocks * block_size) - count;
    uint64_t skip_blocks = skip / block_size;
    uint64_t skip_bytes = skip % block_size;

    uint64_t written_bytes = 0;
    EXT2_DEBUG("writing %ld bytes to ino: %d from %p [skip_blocks: %ld, skip_bytes: %ld, excess_bytes: %ld]", count, inode_number, (void*)source_buffer, skip_blocks, skip_bytes, excess_bytes);

    if (skip_bytes) {
        uint8_t * temp_buffer = (uint8_t *)kmalloc(block_size);
        int64_t skip_read_result = ext2_read_inode_blocks(partition, inode_number, temp_buffer, 1, skip_blocks);
        if (skip_read_result != 1) {
            EXT2_ERROR("Failed to read skip bytes");
            return EXT2_WRITE_FAILED;
        }

        if (inode_size < block_size) {
            memcpy(temp_buffer + skip_bytes, source_buffer, skip_bytes);
        } else {
            memcpy(temp_buffer + skip_bytes, source_buffer, skip_bytes);
        }

        int64_t skip_write_result = ext2_write_inode_blocks(partition, inode_number, temp_buffer, 1, skip_blocks);
        if (skip_write_result != 1) {
            EXT2_ERROR("Failed to write skip bytes");
            return EXT2_WRITE_FAILED;
        }
        kfree(temp_buffer);
        source_buffer += block_size - skip_bytes;
        skip_blocks++;
        written_bytes += block_size - skip_bytes;
        EXT2_WARN("Partial write of %ld bytes", block_size - skip_bytes);
    }
    
    int64_t remaining_bytes = count - written_bytes;

    if (remaining_bytes > 0) {
        EXT2_WARN("Writing second stage, remaining bytes: %ld", remaining_bytes);
        int64_t result = ext2_write_inode_blocks(partition, inode_number, source_buffer, blocks - 1, skip_blocks);
        if (result == EXT2_WRITE_FAILED) return EXT2_WRITE_FAILED;

        if (result < 0) {
            EXT2_ERROR("Failed to write file");
            return EXT2_WRITE_FAILED;
        }

        written_bytes += result * block_size;
        skip_blocks += result;
        EXT2_WARN("Partial write of %ld bytes", result * block_size);
        remaining_bytes = count - written_bytes;
    }

    if (remaining_bytes > 0) {
        EXT2_WARN("Writing third stage, remaining bytes: %ld", remaining_bytes);
        uint8_t * temp_buffer = (uint8_t *)kmalloc(block_size);
        int64_t remaining_read_result = ext2_read_inode_blocks(partition, inode_number, temp_buffer, 1, skip_blocks);
        if (remaining_read_result != 1) {
            EXT2_ERROR("Failed to read remaining bytes");
            return EXT2_WRITE_FAILED;
        }
        memcpy(temp_buffer, source_buffer + written_bytes, remaining_bytes);
        int64_t remaining_write_result = ext2_write_inode_blocks(partition, inode_number, temp_buffer, 1, skip_blocks);
        if (remaining_write_result != 1) {
            EXT2_ERROR("Failed to write remaining bytes");
            return EXT2_WRITE_FAILED;
        }
        kfree(temp_buffer);
        written_bytes += remaining_bytes;
        EXT2_WARN("Partial write of %ld bytes", remaining_bytes);
    }
    
    EXT2_WARN("Wrote %ld bytes", written_bytes);
    return written_bytes;
}