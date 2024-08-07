#ifndef PMM_H
#define PMM_H

#include "omen/libraries/datastructs/bitmap.h"
#include "omen/libraries/datastructs/linked_list.h"

typedef struct {
    void *address;
    size_t size;
    size_t page_size;
    size_t num_levels;
    struct list_head *free_blocks;
    bitmap_t block_info;
} buddy_allocator_t;

#define POW2(n) (1UL << (n))
#define IS_POW2(n) (((n) > 0) && (((n) & ((n) - 1)) == 0))
#define ILOG2(value) ((BITMAP_NUM_BITS - 1UL) - __builtin_clzl(value))

/* allocator->free_blocks size in bytes */
#define BUDDY_FREE_BLOCKS_SIZE(num_levels) (sizeof(struct list_head) * (num_levels))
/* number of blocks in an allocator  */
#define BUDDY_NUM_BLOCKS(num_levels) (POW2(num_levels))
/* allocator->block_info length in unsigned long int  */
#define BUDDY_BLOCK_INFO_LEN(num_levels) (BUDDY_NUM_BLOCKS(num_levels) + (BITMAP_NUM_BITS - 1) / BITMAP_NUM_BITS)
/* allocator->block_info size in bytes */
#define BUDDY_BLOCK_INFO_SIZE(num_levels) (BUDDY_BLOCK_INFO_LEN(num_levels) * (BITMAP_NUM_BITS >> 3))

#define BUDDY_NUM_PAGES(size, page_size) ((size + (page_size - 1)) / page_size)

static inline size_t buddy_sizeof_metadata(size_t num_levels) {
    return sizeof(buddy_allocator_t) + BUDDY_FREE_BLOCKS_SIZE(num_levels) + BUDDY_BLOCK_INFO_SIZE(num_levels);
}

buddy_allocator_t *buddy_create(void *address, size_t size, size_t page_size);
void buddy_init(buddy_allocator_t *allocator, void *address, size_t size, size_t page_size);

void *buddy_alloc(buddy_allocator_t *allocator, size_t size);
void buddy_free(buddy_allocator_t *allocator, void *ptr);

void *buddy_level_alloc(buddy_allocator_t *allocator, unsigned long int level);
void buddy_level_free(buddy_allocator_t *allocator, void *ptr, unsigned long int level);

size_t buddy_largest_available(const buddy_allocator_t *allocator);
size_t buddy_available(const buddy_allocator_t *allocator);
size_t buddy_used(const buddy_allocator_t *allocator);

#endif
