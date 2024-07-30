#include "pmm.h"

#include <stdint.h>
#include <stdio.h>

struct pmm_buddy_node {
    uint8_t level;
    uint8_t used;
    struct pmm_buddy_node *left;
    struct pmm_buddy_node *right;
};

struct pmm_system_memory {
    uint64_t size;
    uint64_t start;
    uint64_t *bitmap;
    struct pmm_buddy_node *root;
};

struct pmm_system_memory *allocate_metadata(uint64_t size, uint64_t start) {
    uint64_t bitmap_size = size / PMM_PAGE_SIZE / 8;
}

void pmm_init(uint64_t size, uint64_t start) { printf("pmm_init\n"); }