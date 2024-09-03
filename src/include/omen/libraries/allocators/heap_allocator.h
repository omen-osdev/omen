#ifndef _HEAP_ALLOCATOR_H
#define _HEAP_ALLOCATOR_H

#include <omen/libraries/std/stdint.h>

void * kmalloc(uint64_t size);
void kfree(void * ptr);

#endif