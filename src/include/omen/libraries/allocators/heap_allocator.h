#ifndef HEAP_H
#define HEAP_H

#include <omen/libraries/std/stdint.h>
#include <omen/managers/mem/vmm.h>
#include <omen/managers/cpu/process.h>

struct stack {
    void * base;
    void * top;
};

void * kmalloc(uint64_t size);
void * kmalloc_standalone(uint64_t size);
void kfree(void* address);

void kstackalloc(struct page_directory* pd, struct stack * stack, uint64_t length);
void kstackfree(struct page_directory* pd, struct stack * stack);
void * malloc(struct page_directory* root, uint64_t size);
void free(struct page_directory* root, void * address);
void * stackalloc(struct page_directory* root, struct stack * stack, uint64_t length);
void stackfree(struct page_directory* root, struct stack * stack);
#endif