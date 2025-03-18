#ifndef HEAP_H
#define HEAP_H

#include <omen/libraries/std/stdint.h>
#include <omen/managers/mem/vmm.h>
#include <omen/managers/cpu/process.h>

void * kmalloc(uint64_t size);
void kfree(void* address);
void * kstackalloc(uint64_t length);
void kstackfree(void* address);
void * malloc(uint64_t size);
void free(void * address);
void * stackalloc(uint64_t length);
void stackfree(void * address);
#endif