#ifndef _BITFIELD_ALLOCATOR_H
#define _BITFIELD_ALLOCATOR_H

#include "../concurrencyLibrary/mutex.h"
#include <stdint.h>

struct bitfield {
    void *data_address;
    uint64_t data_size;

    void *available_address;
    uint64_t available_size;

    uint16_t page_size;

    uint8_t *bitmap;
    uint64_t bitmap_size;
    uint64_t next_index;

    mutex_t lock;
};

void *allocate(struct bitfield *bf, uint64_t size);
void deallocate(struct bitfield *bf, void *address, uint64_t size);
struct bitfield *init(void *data_address, uint64_t data_size, uint16_t page_size);
void debug_bitfield(struct bitfield *bf);
#endif
