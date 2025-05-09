#ifndef HEAP_H
#define HEAP_H

#include <omen/libraries/std/stdint.h>
#include <omen/managers/mem/vmm.h>

#define STACK_GUARD_SIZE 0x1000
#define STACK_GROWTH_SIZE 0x10000

struct stack {
    void * base;
    int flags;
    void * top;
    uint64_t size;
    uint64_t guard_size;
};

void * kmalloc(uint64_t size);
void kfree(void* address);

void kstackalloc(struct page_directory* pd, struct stack * stack, uint64_t length);
void kstackfree(struct page_directory* pd, struct stack * stack);
void stackalloc(struct page_directory* root, struct stack * stack, uint64_t length);
void stackfree(struct page_directory* root, struct stack * stack);
#endif