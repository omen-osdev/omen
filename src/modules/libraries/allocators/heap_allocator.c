#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/managers/mem/pmm.h>
#include <omen/managers/mem/vmm.h>
#include <omen/managers/cpu/process.h>
#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/libraries/concurrency/mutex.h>

void * last_kstack = 0;

void * kmalloc(uint64_t size) {
    void * ptr = allocate_current_vmm(size, PAGE_WRITE_BIT);
    memset(ptr, 0, size);
    return ptr;
}

void kfree(void* address) {
    //free_current_vmm(address);
}

void * kstackalloc(uint64_t length) {
    last_kstack = vmm_create_kernel_stack(get_pml4(), length, PAGE_WRITE_BIT, last_kstack) + length;
    return last_kstack - length;
}

void kstackfree(void* address) {
    kfree(address);
}

void * malloc(uint64_t size) {
    void * ptr = allocate_current_vmm_uspace(size, PAGE_WRITE_BIT);
    memset(ptr, 0, size);
    return ptr;
}

void * pmalloc(struct page_directory* root, uint64_t size) {
    void * ptr = allocate_vmm(root, size, PAGE_WRITE_BIT);
    memset(ptr, 0, size);
    return ptr;
}

void * pstackalloc(struct page_directory* root, uint64_t length) {
    return pmalloc(root, length);
}

void pfree(struct page_directory* root, void * address) {
    free_vmm(root, address);
}

void pstackfree(struct page_directory* root, void * address) {
    pfree(root, address);
}

void free(void * address) {
    free_vmm_uspace(address);
}

void * stackalloc(uint64_t length) {
    if (length % 0x1000) {
        length = (length + 0x1000) & ~0xfff;
    }
    //Return aligned to 0x1000
    void * unaligned_alloc = malloc(length);
    void * aligned = (void*)(((uint64_t)unaligned_alloc + 0xfff) & ~0xfff);
    return aligned;
}

void stackfree(void * address) {
    free(address);
}