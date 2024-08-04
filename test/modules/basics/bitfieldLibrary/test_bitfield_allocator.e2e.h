#ifndef TEST_BITFIELD_ALLOCATOR_H
#define TEST_BITFIELD_ALLOCATOR_H

#include "modules/basics/bitfieldLibrary/bitfield_allocator.h"

#include "unity.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Struct definitions
struct _allocation_data {
    struct bitfield *bf;
    void *address;
    uint64_t size;
};

struct _allocation_tracker {
    void *address;
    uint64_t size;
    uint8_t freed;
    uint64_t expected_fragmentation;
    struct _allocation_tracker *next;
};

struct _allocation_metadata {
    void *allocation_control_structure;
    uint64_t iterations;
    uint64_t allocated;
    uint64_t page_size;
    struct _allocation_tracker *tracker;
    pthread_mutex_t tracker_lock;
};

// Private functions
void *_allocate_stub(void *data);
void *_deallocate_stub(void *data);
void *_check_stub(void *data);
void _add_allocation(struct _allocation_metadata *allocation, void *address, uint64_t size);
void _mark_as_freed(struct _allocation_metadata *allocation, void *address);
void _worker_thread(void *arg);

#endif