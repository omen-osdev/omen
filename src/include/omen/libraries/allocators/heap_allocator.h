#ifndef HEAP_H
#define HEAP_H

#include <omen/libraries/std/stdint.h>
#include <omen/managers/cpu/process.h>
#include <omen/libraries/concurrency/mutex.h>
//This code comes from https://github.com/kot-org/Kot/blob/main/Sources/Kernel/Src/heap/heap.h
//Thank you Konect!!!

struct heap_segment_header {
    uint8_t free;
    uint64_t length;
    struct heap_segment_header* next;
    struct heap_segment_header* last;
    uint8_t isStack;
    uint32_t signature;
} __attribute__((aligned(0x10)));

struct heap {
    void* heapEnd;
    void* lastStack;
    struct heap_segment_header* lastSegment;
    struct heap_segment_header* mainSegment;
    uint64_t totalSize;
    uint64_t usedSize;
    uint64_t freeSize;
    uint8_t isKernel;
    uint8_t ready;
    spinlock_t heap_lock;
    spinlock_t stack_lock;
};

extern struct heap kernelGlobalHeap;
extern struct heap userGlobalHeap;

void init_heap();

void * kmalloc(uint64_t size);
void kfree(void* address);
void * kcalloc(uint64_t num, uint64_t size);
void * krealloc(void* buffer, uint64_t size);
void * kstackalloc(uint64_t length);

void * smalloc(uint64_t size);
void sfree(void* address);

void * sigmalloc(uint64_t size);
void sigfree(void* address);

void create_user_heap(process_t * task, struct heap * cheap);
void * umalloc(process_t * task, uint64_t size);
void ufree(process_t * task, void* address);
void * ucalloc(process_t * task, uint64_t num, uint64_t size);
void * urealloc(process_t * task, void* buffer, uint64_t size);
void * ustackalloc(process_t * task, uint64_t length);
int heap_safeguard(struct heap * cheap);
//void debug_heap();
//void walk_heap();
#endif