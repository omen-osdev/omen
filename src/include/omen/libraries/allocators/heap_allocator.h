#ifndef HEAP_H
#define HEAP_H

#include <omen/libraries/std/stdint.h>
#include <omen/managers/mem/vmm.h>
#include <omen/managers/cpu/process.h>

void * kmalloc(uint64_t size);
void kfree(void* address);
void * kstackalloc(uint64_t length);
void kstackfree(void* address);
void * malloc(struct page_directory * pml4, uint64_t size);
void free(struct page_directory * pml4, void * address);
void * stackalloc(struct page_directory * pml4, uint64_t length);
void stackfree(struct page_directory * pml4, void * address);
#endif