#ifndef HEAP_H
#define HEAP_H

#include <omen/libraries/std/stdint.h>
#include <omen/managers/mem/vmm.h>

struct stack {
    void * base;
    int flags;
    void * top;
};

void * kmalloc(uint64_t size);
void kfree(void* address);

void kstackalloc(struct page_directory* pd, struct stack * stack, uint64_t length);
void kstackfree(struct page_directory* pd, struct stack * stack);
void * stackalloc(struct page_directory* root, struct stack * stack, uint64_t length);
void stackfree(struct page_directory* root, struct stack * stack);
#endif