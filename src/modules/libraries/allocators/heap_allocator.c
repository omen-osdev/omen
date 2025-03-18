#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/managers/mem/pmm.h>
#include <omen/managers/mem/vmm.h>
#include <omen/managers/cpu/process.h>
#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/libraries/concurrency/mutex.h>

void * kmalloc(uint64_t size) {
    void * ptr = allocate_current_vmm(size, PAGE_WRITE_BIT);
    memset(ptr, 0, size);
    return ptr;
}

void kfree(void* address) {
    free_current_vmm(address);
}

void * kstackalloc(uint64_t length) {
    return kmalloc(length);
}

void kstackfree(void* address) {
    kfree(address);
}

void * malloc(uint64_t size) {
    void * ptr = allocate_current_vmm_uspace(size, PAGE_WRITE_BIT);
    memset(ptr, 0, size);
    return ptr;
}

void free(void * address) {
    free_vmm_uspace(address);
}

void * stackalloc(uint64_t length) {
    return malloc(length);
}

void stackfree(void * address) {
    free(address);
}