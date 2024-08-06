#include "omen/modules/pmm/pmm.h"
#include "omen/libraries/datastructs/bitmap.h"
#include "omen/libraries/datastructs/list.h"

static inline unsigned long int buddy_page_index(const buddy_allocator_t *allocator, const void *ptr) {
    return (ptr - allocator->address) / allocator->page_size;
}

static inline unsigned long int buddy_block_index(const buddy_allocator_t *allocator, const void *ptr,
                                                  unsigned long int level) {
    return POW2(level) + (buddy_page_index(allocator, ptr) >> (allocator->num_levels - 1 - level)) - 1UL;
}

/** free_bit - Get the index of the free bit for the given block_index
 * @index: block index
 *
 * Return: index of the free bit in the allocator->block_info bitmap
 */
static inline unsigned long int free_bit(unsigned long int block_index) { return (block_index - 1) >> 1; }

/** split_bit - Get the index of the split bit for the given block_index
 * @allocator: pointer to the allocator
 * @index: block index
 *
 * Return: index of the split bit in the allocator->block_info bitmap
 */
static inline unsigned long int split_bit(const buddy_allocator_t *allocator, unsigned long int index) {
    return index + (BUDDY_NUM_BLOCKS(allocator->num_levels) >> 1);
}

/** to_buddy - Returns a pointer to the buddy of a block
 * @allocator: pointer to the allocator structure
 * @ptr: pointer to the block
 * @level: block level (0 < level < allocator->num_levels)
 */
static inline void *to_buddy(const buddy_allocator_t *allocator, const void *ptr, unsigned long int level) {
    return ((ptr - allocator->address) ^ (allocator->size >> level)) + allocator->address;
}

/** size_to_level - Get the minimum allocation level needed to hold the given size
 * @allocator: pointer to the allocator
 * @size: size of the allocation in bytes
 *
 * Return: minimum allocation level needed
 */
static inline unsigned int long size_to_level(const buddy_allocator_t *allocator, size_t size) {
    if (size < allocator->page_size)
        return allocator->num_levels - 1;

    // Round up to next power of two
    size = 1 << (BITMAP_NUM_BITS - __builtin_clzl(size - 1));

    // Delta between the number of levels and the first set bit in the size
    return ILOG2(allocator->size) - ILOG2(size);
}

/** buddy_level_alloc - Allocate a block of memory of at least 2^level pages of size
 * @allocator: pointer to the allocator
 * @level: size of the allocation is 2^level pages
 */
void *buddy_level_alloc(buddy_allocator_t *allocator, unsigned long int level) {
    struct list_head *block_ptr = NULL;
    struct list_head *buddy_ptr = NULL;

    long int block_level;
    for (block_level = level; block_level >= 0; block_level--) {
        if (!list_empty(&allocator->free_blocks[block_level])) {
            block_ptr = allocator->free_blocks[block_level].next;
            list_del(block_ptr);
            break;
        }
    }

    if (!block_ptr)
        return block_ptr;

    unsigned long int block_index = buddy_block_index(allocator, block_ptr, block_level);

    // Do we need to split blocks?
    if ((unsigned long int)block_level != level) {
        // Split until we reach the requested level
        while ((unsigned long int)block_level < level) {
            // Mark the block as split
            bitmap_set(allocator->block_info, split_bit(allocator, block_index));

            // Block at level 0 does not form a pair
            if (block_level > 0)
                // Mark pair as allocated (free_bit=1 -> both allocated / both free)
                // we toggle the bit because it might be the case that its buddy is allocated as well
                bitmap_toggle(allocator->block_info, free_bit(block_index));

            buddy_ptr = (struct list_head *)to_buddy(allocator, block_ptr, block_level + 1);

            // Add right buddy to the free list
            list_add(buddy_ptr, &allocator->free_blocks[block_level + 1]);

            // Get index of right child
            block_index = (block_index << 1) + 1;
            block_level++;
        }
    }

    if (level > 0)
        bitmap_toggle(allocator->block_info, free_bit(block_index));

    return block_ptr;
}

/** buddy_level_free - Free a previous allocation of known size
 * @allocator: pointer to the allocator
 * @ptr: a pointer to the block of memory to deallocate
 * @level: size of the allocation is 2^level pages
 */
void buddy_level_free(buddy_allocator_t *allocator, void *ptr, unsigned long int level) {
    void *buddy_ptr = to_buddy(allocator, ptr, level);

    unsigned long int block_index = buddy_block_index(allocator, ptr, level);

    if (level > 0)
        bitmap_toggle(allocator->block_info, free_bit(block_index));

    while (level > 0 && !bitmap_get(allocator->block_info, free_bit(block_index))) {
        // Merge the buddies (clear the split bit)
        bitmap_clear(allocator->block_info, split_bit(allocator, block_index));

        // Remove the buddy from the free list
        list_del(buddy_ptr);

        // Get the index of the parent
        block_index = (block_index - 1) >> 1;
        level--;

        // Make sure we are using the left buddy
        if (buddy_ptr < ptr)
            ptr = buddy_ptr;

        buddy_ptr = to_buddy(allocator, ptr, level);

        // Toggle the free bit of the parent
        if (level > 0)
            bitmap_toggle(allocator->block_info, free_bit(block_index));
    }

    // Clear the split bit
    bitmap_clear(allocator->block_info, split_bit(allocator, block_index));

    // Add the block to the free list
    list_add(ptr, &allocator->free_blocks[level]);
}

/** buddy_init - Initialize an already existing allocator
 * @allocator: pointer to the allocator
 * @address: address of the memory region to manage
 * @size: size of the memory region to manage
 * @page_size: minimum block size of the allocator
 *
 * Both @size and @page_size must be a power of 2
 */
void buddy_init(buddy_allocator_t *allocator, void *address, size_t size, size_t page_size) {
    unsigned long int num_levels = ILOG2(size) - ILOG2(page_size) + 1;

    allocator->address = address;
    allocator->size = size;
    allocator->page_size = page_size;
    allocator->num_levels = num_levels;
    allocator->free_blocks = (void *)allocator + sizeof(buddy_allocator_t);
    allocator->block_info =
        (void *)allocator + sizeof(buddy_allocator_t) + BUDDY_FREE_BLOCKS_SIZE(allocator->num_levels);

    for (size_t i = 0; i < allocator->num_levels; i++) {
        INIT_LIST_HEAD(&allocator->free_blocks[i]);
    }

    // Add the level 0 block (whole memory) to the free list
    list_add((struct list_head *)address, &allocator->free_blocks[0]);

    // Mark everything as non split and not allocated
    for (size_t i = 0; i < BUDDY_BLOCK_INFO_LEN(allocator->num_levels); i++) {
        allocator->block_info[i] = 0;
    }
}

/** buddy_create - Create a new buddy allocator with embedded metadata
 * @address: address of the memory region to manage
 * @size: size of the memory region to manage
 * @page_size: minimum block size of the allocator
 *
 * Both @size and @page_size must be a power of 2
 *
 * Return: pointer to the allocator on success, NULL on error
 */
buddy_allocator_t *buddy_create(void *address, size_t size, size_t page_size) {
    if (address == NULL || !IS_POW2(size) || !IS_POW2(page_size))
        return NULL;

    unsigned long int num_levels = ILOG2(size) - ILOG2(page_size) + 1;
    size_t metadata_size = buddy_sizeof_metadata(num_levels);

    // We create a temporal allocator at the end of the memory region so that we can safely allocate the blocks at the
    // beginning of the region where the final allocator will be located
    buddy_allocator_t *tmp = (address + size) - metadata_size;
    buddy_init(tmp, address, size, page_size);

    // Allocate enough pages for the metadata (the allocated pages will be at the beginning of the memory region)
    for (size_t i = 0; i < BUDDY_NUM_PAGES(metadata_size, tmp->page_size); i++) {
        buddy_level_alloc(tmp, tmp->num_levels - 1);
    }

    // Now create the actual allocator in the beginning of the memory region
    buddy_allocator_t *allocator = address;
    allocator->address = address;
    allocator->size = tmp->size;
    allocator->page_size = tmp->page_size;
    allocator->num_levels = tmp->num_levels;
    allocator->free_blocks = address + sizeof(buddy_allocator_t);
    allocator->block_info = address + sizeof(buddy_allocator_t) + BUDDY_FREE_BLOCKS_SIZE(allocator->num_levels);

    // Copy the block info
    for (size_t i = 0; i < BUDDY_BLOCK_INFO_LEN(num_levels); i++) {
        allocator->block_info[i] = tmp->block_info[i];
    }

    // Move the free blocks lists
    for (size_t i = 0; i < num_levels; i++) {
        INIT_LIST_HEAD(&allocator->free_blocks[i]);
        list_splice(&tmp->free_blocks[i], &allocator->free_blocks[i]);
    }

    return allocator;
}

/** buddy_alloc - Allocate a block of memory of at least size bytes
 * @allocator: pointer to the allocator
 * @size: requested size (doesn't have to be a power of 2)
 */
void *buddy_alloc(buddy_allocator_t *allocator, size_t size) {
    if (size > allocator->size)
        return NULL;

    unsigned long int level = size_to_level(allocator, size);
    return buddy_level_alloc(allocator, level);
}

/** buddy_free - Free a previous allocation
 * @allocator: pointer to the allocator
 * @ptr: a pointer to the block of memory to deallocate
 */
void buddy_free(buddy_allocator_t *allocator, void *ptr) {
    if (ptr == NULL)
        return;
    if (ptr < allocator->address || ptr > allocator->address + allocator->size)
        return;

    unsigned long int block_index = buddy_block_index(allocator, ptr, allocator->num_levels - 1);

    // Determine level
    for (long int level = allocator->num_levels - 1; level >= 0; level--) {
        // Get index of parent
        block_index = (block_index - 1) >> 1;

        if (bitmap_get(allocator->block_info, split_bit(allocator, block_index))) {
            buddy_level_free(allocator, ptr, level);
            return;
        }
    }

    // Deallocate whole memory region
    buddy_level_free(allocator, ptr, 0);
}

/** buddy_largest_available - Return the largest block size available
 * @allocator: pointer to the allocator
 *
 * Return: the largest block size available in bytes
 */
size_t buddy_largest_available(const buddy_allocator_t *allocator) {
    for (unsigned long int level = 0; level < allocator->num_levels; level++) {
        if (!list_empty(&allocator->free_blocks[level])) {
            return allocator->size >> level;
        }
    }
    return 0;
}

/** buddy_available - Return the total available memory
 * @allocator: pointer to the allocator
 *
 * Return: the total available memory in bytes
 */
size_t buddy_available(const buddy_allocator_t *allocator) {
    size_t sum = 0;
    for (unsigned long int level = 0; level < allocator->num_levels; level++) {
        struct list_head *node;
        int count = 0;
        list_for_each(node, &allocator->free_blocks[level]) { count++; }
        sum += count * (allocator->size >> level);
    }
    return sum;
}

/** buddy_available - Return the total used memory
 * @allocator: pointer to the allocator
 *
 * Return: the total used memory in bytes
 */
size_t buddy_used(const buddy_allocator_t *allocator) { return allocator->size - buddy_available(allocator); }
