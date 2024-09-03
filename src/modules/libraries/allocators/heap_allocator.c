#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/managers/mem/vmm.h>
#include <omen/managers/mem/pmm.h>
#include <omen/libraries/std/stdint.h>

void * kmalloc(uint64_t size) {
    void * buffer = pmm_alloc(size);
    uint64_t pages = size / PAGE_SIZE;
    if (size % PAGE_SIZE) {
        pages++;
    }

    for (uint64_t i = 0; i < pages; i++) {
        map_current_memory((void*)((uint64_t)buffer + i * PAGE_SIZE), (void*)((uint64_t)buffer + i * PAGE_SIZE), 0x3);
    }

    return buffer;
}

void kfree(void * ptr) {
    //TODO: free pages
    (void)ptr;
}