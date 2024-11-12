#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/managers/mem/pmm.h>
#include <omen/managers/mem/vmm.h>
#include <omen/managers/cpu/process.h>
#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/libraries/concurrency/mutex.h>
//This code comes from https://github.com/kot-org/Kot/blob/main/Sources/Kernel/Src/heap/heap.cpp
//Thank you Konect!!!
#define HEAP_SIGNATURE  0xcafebabe

#define LOCK_HEAP(heap) (spinlock_lock(&((heap)->heap_lock)))
#define UNLOCK_HEAP(heap) (spinlock_unlock(&((heap)->heap_lock)))
#define UNLOCK_RETURN(heap, ret) {spinlock_unlock(&((heap)->heap_lock)); return ret;}
#define LOCK_STACK(heap) (spinlock_lock(&((heap)->stack_lock)))
#define UNLOCK_STACK(heap) (spinlock_unlock(&((heap)->stack_lock)))
#define UNLOCK_RETURN_STACK(heap, ret) {spinlock_unlock(&((heap)->stack_lock)); return ret;}

struct heap * kernel_heap = 0;

struct heap * get_kernel_heap() {
    return kernel_heap;
}

void set_kernel_heap(struct heap * heap) {
    kernel_heap = heap;
}

void _initHeap(struct heap * cheap, struct page_directory * pml4, void* heapStart, void * heapEnd, void* stackStart, void * stackEnd, uint8_t user) {
    uint8_t perms = PAGE_WRITE_BIT;
    if (user) perms |= PAGE_USER_BIT;

    cheap->pd = pml4;
    cheap->heapStart = heapStart;
    cheap->heapEnd = heapEnd - (uint64_t)PAGE_SIZE;
    cheap->stackStart = stackStart;
    cheap->stackEnd = stackEnd - (uint64_t)PAGE_SIZE;
    cheap->isKernel = !user;
    cheap->ready = 1;
}

struct heap * init_heap(struct page_directory * pml4, uint8_t user_access, uint64_t heapStart, uint64_t heapEnd, uint64_t stackStart, uint64_t stackEnd) {
    struct heap * cheap = (struct heap*)pmm_alloc(sizeof(struct heap));
    if (cheap == NULL) {
        panic("ERROR: Could not allocate heap\n");
    }
    memset(cheap, 0, sizeof(struct heap));
    
    map_memory(pml4, (void*)cheap, cheap, 0x1000, PAGE_WRITE_BIT);

    _initHeap(cheap, pml4, (void*)heapStart, (void*)heapEnd, (void*)stackStart, (void*)stackEnd, user_access);

    return cheap;
}

void * malloc(struct heap * heap, uint64_t size) {
    LOCK_HEAP(heap);
    if (heap->ready == 0) {
        panic("ERROR: Heap not ready\n");
    }

    //void * result = _malloc(&kernelGlobalHeap, size);
    uint64_t last_address = (uint64_t)heap->heapEnd;

    uint64_t pages = size / PAGE_SIZE;
    if (size % PAGE_SIZE) pages++;

    for (uint64_t i = 0; i < pages; i++) {
        void * buffer = pmm_alloc(PAGE_SIZE);
        if (buffer == NULL) {
            panic("ERROR: Could not allocate page for heap\n");
        }

        memset(buffer, 0, PAGE_SIZE);
        map_memory(heap->pd, heap->heapEnd, buffer, PAGE_SIZE, PAGE_WRITE_BIT);

        heap->heapEnd = (void*)((uint64_t)heap->heapEnd - (uint64_t)PAGE_SIZE);
    }

    UNLOCK_HEAP(heap);
    return (void*)last_address;
}

void free(struct heap * heap, void* address) {
    //TODO: Implement unmap on free
    LOCK_HEAP(heap);
    if (heap->ready == 0) {
        panic("ERROR: Heap not ready\n");
    }

    //_free(&kernelGlobalHeap, address);
    panic("kfree not implemented\n");
    UNLOCK_HEAP(heap);
}

void * stackalloc(struct heap * heap, uint64_t length) {
    LOCK_STACK(heap);
    if (heap->ready == 0) {
        panic("ERROR: Heap not ready\n");
    }

    //void * result = _stackalloc(&kernelGlobalHeap, length);
    uint64_t last_address = (uint64_t)heap->stackEnd;

    uint64_t pages = length / PAGE_SIZE;
    if (length % PAGE_SIZE) pages++;

    for (uint64_t i = 0; i < pages; i++) {
        void * buffer = pmm_alloc(PAGE_SIZE);
        if (buffer == NULL) {
            panic("ERROR: Could not allocate page for stack\n");
        }

        memset(buffer, 0, PAGE_SIZE);
        map_memory(heap->pd, heap->stackEnd, buffer, PAGE_SIZE, PAGE_WRITE_BIT);

        heap->stackEnd = (void*)((uint64_t)heap->stackEnd - (uint64_t)PAGE_SIZE);
    }

    UNLOCK_STACK(heap);
    return (void*)last_address;
}

void * kmalloc(uint64_t size) {
    return malloc(kernel_heap, size);
}

void kfree(void* address) {
    free(kernel_heap, address);
}

void * kstackalloc(uint64_t length) {
    return stackalloc(kernel_heap, length);
}
