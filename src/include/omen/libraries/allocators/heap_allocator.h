#ifndef HEAP_H
#define HEAP_H

#include <omen/libraries/std/stdint.h>
#include <omen/managers/mem/vmm.h>
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
    struct page_directory * pd;
    void* heapStart;
    void* heapEnd;
    void* stackStart;
    void* stackEnd;
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

struct heap * init_heap(struct page_directory * pml4, uint8_t user_access, uint64_t heapStart, uint64_t heapEnd, uint64_t stackStart, uint64_t stackEnd);
void * malloc(struct heap * heap, uint64_t size);
void * kmalloc(uint64_t size);
void free(struct heap * heap, void* address);
void kfree(void* address);
void * stackalloc(struct heap * heap, uint64_t length);
void * kstackalloc(uint64_t length);
struct heap * get_kernel_heap();
void set_kernel_heap(struct heap * heap);
#endif