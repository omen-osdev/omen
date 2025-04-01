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
    //free_current_vmm(address);
}

void kstackalloc(struct stack * stack, uint64_t length) {

    uint64_t pages = length / 0x1000;
    if (length % 0x1000) {
        pages++;
    }

    uint64_t stack_base;
    uint64_t stack_top = vmm_create_kernel_stack(get_pml4(), pages, PAGE_WRITE_BIT, &stack_base);

    stack->base = (void*)stack_base;
    stack->top = (void*)stack_top;
}

void kstackfree(struct stack * stack) {
    kfree(stack->base);
}

void * malloc(uint64_t size) {
    void * ptr = allocate_current_vmm_uspace(size, VMM_REGION_U_HEAP, PAGE_WRITE_BIT);
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

void * stackalloc(struct stack * stack, uint64_t length) {
    uint64_t pages = length / 0x1000;

    if (length % 0x1000) {
        pages++;
    }

    uint64_t base = allocate_current_vmm_uspace(pages*0x1000, VMM_REGION_U_STACK, PAGE_WRITE_BIT);
    memset((void*)base, 0, pages*0x1000);
    uint64_t top = (uint64_t)(base+pages*0x1000)-0x10;
    //if unaligned_alloc is not 16-byte aligned, align it by subtracting the difference
    if (top % 0x10) {
        top -= top % 0x10;
    }
    top -= 0x8;
    
    stack->base = (void*)(base);
    stack->top = (void*)(top);
}

void stackfree(struct stack * stack) {
    free(stack->base);
}