#include "bitfield_allocator.h"
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define BYTES_TO_PAGES(bytes, page_size) ((bytes) / (page_size) + ((bytes) % (page_size) ? 1 : 0))

uint8_t _is_free(struct bitfield *bf, uint64_t index) { return !(bf->bitmap[index / 8] & (1 << (index % 8))); }

uint8_t _fits(struct bitfield *bf, uint64_t index, uint64_t pages) {
    for (uint64_t i = index; i < (index + pages); i++) {
        if (!_is_free(bf, i)) {
            return 0;
        }
    }

    return 1;
}

uint64_t _get_free_index(struct bitfield *bf, uint64_t pages) {
    for (uint64_t i = bf->next_index; i < bf->bitmap_size; i++) {
        if (_fits(bf, i, pages)) {
            return i;
        }
    }

    for (uint64_t i = 0; i < bf->next_index; i++) {
        if (_fits(bf, i, pages)) {
            return i;
        }
    }

    return 0; // A zero here is considered an error
}

void _lock_index(struct bitfield *bf, uint64_t index, uint64_t pages) {
    for (uint64_t i = index; i < (index + pages); i++) {
        bf->bitmap[i / 8] |= (1 << (i % 8));
    }
}

void _un_lock_index(struct bitfield *bf, uint64_t index, uint64_t pages) {
    for (uint64_t i = index; i < (index + pages); i++) {
        bf->bitmap[i / 8] &= ~(1 << (i % 8));
    }
}

void *allocate(struct bitfield *bf, uint64_t size) {

    mutex_lock(&bf->lock);
    // Check if the size is bigger than the available size
    if (size > bf->available_size) {
        printf("Size is bigger than the available size\n");
        return NULL;
    }

    // Check if the size is bigger than the page size
    if (size < bf->page_size) {
        size = bf->page_size;
    }

    // Lock the bitfield

    uint64_t pages = BYTES_TO_PAGES(size, bf->page_size);
    uint64_t index = _get_free_index(bf, pages);

    if (index == 0) {

        mutex_unlock(&bf->lock);
        printf("No available pages\n");
        return NULL;
    }

    _lock_index(bf, index, pages);

    if (index == bf->next_index) {
        bf->next_index = index + pages;
    }

    void *address = bf->available_address + (index * bf->page_size);
    mutex_unlock(&bf->lock);
    return address;
}

void deallocate(struct bitfield *bf, void *address, uint64_t size) {

    mutex_lock(&bf->lock);

    if (address < bf->available_address || address >= (bf->available_address + bf->available_size)) {
        return;
    }

    uint64_t index = ((uint64_t)address - (uint64_t)bf->available_address) / bf->page_size;
    uint64_t pages = BYTES_TO_PAGES(size, bf->page_size);

    _un_lock_index(bf, index, pages);

    if (index < bf->next_index) {
        bf->next_index = index;
    }

    mutex_unlock(&bf->lock);
}

struct bitfield *init(void *data_address, uint64_t data_size, uint16_t page_size) {

    if (data_size < sizeof(struct bitfield) || page_size == 0 || data_size < page_size) {
        return NULL;
    }

    struct bitfield *bf = (struct bitfield *)data_address;
    uint64_t bitmap_size = BYTES_TO_PAGES(data_size, page_size) / 8;
    uint64_t locked_pages = BYTES_TO_PAGES(sizeof(struct bitfield) + bitmap_size, page_size);

    bf->data_address = data_address;
    bf->data_size = data_size;
    bf->page_size = page_size;
    bf->bitmap = (uint8_t *)(data_address + sizeof(struct bitfield));
    bf->bitmap_size = bitmap_size;
    bf->available_address = data_address + sizeof(struct bitfield) + bitmap_size;
    bf->available_size = data_size - sizeof(struct bitfield) - bitmap_size;

    mutex_init(&bf->lock);

    // Lock the pages used by the bitfield
    for (uint64_t i = 0; i < locked_pages; i++) {
        bf->bitmap[i] = 1;
    }

    // Set the rest of the bitmap to 0
    for (uint64_t i = locked_pages; i < bitmap_size; i++) {
        bf->bitmap[i] = 0;
    }

    bf->next_index = locked_pages;
    return bf;
}

void debug_bitfield(struct bitfield *bf) {
    uint64_t total_size = bf->data_size;
    uint64_t allocated_space = 0;
    uint64_t free_space = 0;

    for (uint64_t i = 0; i < bf->bitmap_size; i++) {
        for (uint64_t j = 0; j < 8; j++) {
            if (bf->bitmap[i] & (1 << j)) {
                allocated_space += bf->page_size;
            } else {
                free_space += bf->page_size;
            }
        }
    }

    printf("Total size: %lu\n", total_size);
    printf("Allocated space: %lu\n", allocated_space);
    printf("Free space: %lu\n", free_space);
}