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
void kfree(void* address);
void kstackalloc(struct stack * stack, uint64_t length);
void kstackfree(struct stack * stack);
void * malloc(uint64_t size);
void free(void * address);
void * stackalloc(struct stack * stack, uint64_t length);
void stackfree(struct stack * stack);
#endif