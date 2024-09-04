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

struct heap kernelGlobalHeap;
struct heap kernelSleepHeap;
struct heap kernelSignalHeap;

void * _malloc(struct page_directory* pml4, struct heap * cheap, uint64_t size);
void expand_heap(struct page_directory* pml4, struct heap * cheap, uint64_t length);

void _initHeap(struct page_directory * pml4, struct heap * cheap, void* heapStart, void * heapEnd, void* stackStart, void * stackEnd, uint8_t user) {
    uint8_t perms = PAGE_WRITE_BIT;
    if (user) perms |= PAGE_USER_BIT;

    cheap->heapEnd = heapEnd - (uint64_t)PAGE_SIZE;
    request_current_page_at(cheap->heapEnd, perms);
    cheap->lastStack = stackEnd - (uint64_t)PAGE_SIZE;
    request_current_page_at(cheap->lastStack, perms);

    struct page_directory* pd = get_pml4();
    if (pd != pml4) {
        void * physical = virtual_to_physical(pd,  cheap->heapEnd);
        map_memory(pml4, cheap->heapEnd, physical, perms);
        physical = virtual_to_physical(pd, cheap->lastStack);
        map_memory(pml4, cheap->lastStack, physical, perms);
    }

    cheap->mainSegment = (struct heap_segment_header*)((uint64_t)cheap->heapEnd + ((uint64_t)PAGE_SIZE -sizeof(struct heap_segment_header)));
    cheap->mainSegment->length = 0;
    cheap->mainSegment->free = 0;
    cheap->mainSegment->isStack = 0;
    cheap->mainSegment->last = 0;
    cheap->mainSegment->signature = HEAP_SIGNATURE;
    cheap->mainSegment->next = (struct heap_segment_header*)((uint64_t)cheap->mainSegment - (uint64_t)PAGE_SIZE + sizeof(struct heap_segment_header));

    cheap->mainSegment->next->free = 1;
    cheap->mainSegment->next->isStack = 0;
    cheap->mainSegment->next->length = (uint64_t)PAGE_SIZE - sizeof(struct heap_segment_header) - sizeof(struct heap_segment_header);
    cheap->mainSegment->next->last = cheap->mainSegment;
    cheap->mainSegment->next->next = 0;
    cheap->mainSegment->next->signature = HEAP_SIGNATURE;
    cheap->lastSegment = cheap->mainSegment->next;

    cheap->totalSize += PAGE_SIZE;
    cheap->freeSize += PAGE_SIZE;
    cheap->isKernel = !user;
    cheap->heap_lock.lock = 0;
    cheap->stack_lock.lock = 0;
    cheap->ready = 1;
}

int heap_safeguard(struct heap * cheap) {
    return cheap->ready;
}

void *_calloc(struct page_directory* pml4, struct heap * cheap, uint64_t num, uint64_t size) {
    if (!cheap->ready) return 0;
    void *ptr = _malloc(pml4, cheap, num * size);
    memset(ptr, 0, num * size);
    return ptr;
}

struct heap_segment_header* GetHeapSegmentHeader(void* address) {
    return (struct heap_segment_header*)(void*)((uint64_t)address - sizeof(struct heap_segment_header));
}

void dump_segment(struct heap_segment_header* segment) {
    printf("Segment %p:\n", segment);
    printf("Length: %d\n", segment->length);
    printf("Free: %d\n", segment->free);
    printf("Last: %p\n", segment->last);
    printf("Next: %p\n", segment->next);
    printf("Signature at: %p value: %x\n", &(segment->signature), segment->signature);
}

void debug_heap(struct heap * cheap) {
    printf("Heap debug:\n");
    printf("Total size: %d\n", cheap->totalSize);
    printf("Free size: %d\n", cheap->freeSize);
    printf("Used size: %d\n", cheap->usedSize);
    printf("Heap end: %p\n", cheap->heapEnd);
    printf("Main segment:\n");
    dump_segment(cheap->mainSegment);
    printf("Last segment:\n");
    dump_segment(cheap->lastSegment);
}

struct heap_segment_header* splitSegment(struct heap * cheap, struct heap_segment_header* segment, uint64_t size) {
    if (segment->length <= size + sizeof(struct heap_segment_header)) return 0;
    if (segment->signature != HEAP_SIGNATURE) {cheap->ready = 0; panic("Invalid heap segment signature split");}
    struct heap_segment_header* newSegment = (struct heap_segment_header*)(void*)((uint64_t)segment + segment->length - size);
    memset(newSegment, 0, sizeof(struct heap_segment_header));
    newSegment->free = 1;
    newSegment->length = size;
    newSegment->isStack = 0;
    newSegment->next = segment;
    newSegment->signature = HEAP_SIGNATURE;
    newSegment->last = segment->last;

    if (segment->next == 0) cheap->lastSegment = segment;
    if (segment->last != 0) segment->last->next = newSegment;
    segment->last = newSegment;
    segment->length -= size + sizeof(struct heap_segment_header);

    return newSegment;
}

void walk_heap(struct heap * cheap) {
    struct heap_segment_header* currentSegment = (struct heap_segment_header*)cheap->mainSegment;
    while(1) {
        if (currentSegment->signature != HEAP_SIGNATURE) {printf("***************** HEAP SIGNATURE INVALID BELOW *****************\n");}
        dump_segment(currentSegment);
        if (currentSegment->next == 0) break;
        currentSegment = currentSegment->next;
    }
}

void* _malloc(struct page_directory* pml4, struct heap * cheap, uint64_t size) {
    if (!cheap->ready) return 0;
    if (size == 0) return 0;

    if ((size % 0x10) > 0) {
        size -= (size % 0x10);
        size += 0x10;
    }

    struct heap_segment_header* currentSegment = (struct heap_segment_header*)cheap->mainSegment;
    uint64_t sizeWithHeader = size + sizeof(struct heap_segment_header);

    while(1) {
        if (currentSegment->signature != HEAP_SIGNATURE) {cheap->ready = 0; walk_heap(cheap); panic("Invalid heap segment signature malloc");}
        if (currentSegment->free) {
            if (currentSegment->length > sizeWithHeader) {
                currentSegment = splitSegment(cheap, currentSegment, size);
                currentSegment->free = 0;
                cheap->usedSize += currentSegment->length + sizeof(struct heap_segment_header);
                cheap->freeSize -= currentSegment->length + sizeof(struct heap_segment_header);
                return (void*)((uint64_t)currentSegment + sizeof(struct heap_segment_header));
            } else if (currentSegment->length == size) {
                currentSegment->free = 0;
                cheap->usedSize += currentSegment->length + sizeof(struct heap_segment_header);
                cheap->freeSize -= currentSegment->length + sizeof(struct heap_segment_header);
                return (void*)((uint64_t)currentSegment + sizeof(struct heap_segment_header));
            }
        }
        if (currentSegment->next == 0) break;
        if (currentSegment->next == currentSegment) {cheap->ready = 0; panic("Current segment cycle\n");}
        currentSegment = currentSegment->next;
    }

    expand_heap(pml4, cheap, size);
    return _malloc(pml4, cheap, size);
}

void MergeThisToNext(struct heap_segment_header* header){
    struct heap_segment_header* next = header->next;
    next->length += header->length + sizeof(struct heap_segment_header);
    next->last = header->last;
    header->last->next = next;

    memset(header, 0, sizeof(struct heap_segment_header));
}

void MergeLastToThis(struct heap_segment_header* header){
    struct heap_segment_header* last = header->last;
    header->length += last->length + sizeof(struct heap_segment_header);
    header->last = last->last;
    header->last->next = header;

    memset(last, 0, sizeof(struct heap_segment_header));
}

void MergeLastAndThisToNext(struct heap_segment_header* header){
    MergeLastToThis(header);
    MergeThisToNext(header);
}

void _free(struct heap * cheap, void* address) {
    if (!cheap->ready) return;
    if (address == 0) return;

    struct heap_segment_header* header = (struct heap_segment_header*)((uint64_t)address - sizeof(struct heap_segment_header));
    if (header->signature != HEAP_SIGNATURE) {cheap->ready = 0; panic("Invalid free, signature");}
    if (header->free) {cheap->ready = 0; panic("Invalid free, double free");}
    header->free = 1;
    cheap->freeSize += header->length + sizeof(struct heap_segment_header);
    cheap->usedSize -= header->length + sizeof(struct heap_segment_header);

    if (header->next != 0 && header->last != 0) {
        if (header->next->free && header->last->free) {
            MergeLastAndThisToNext(header);
            return;
        }
    }

    if (header->last != 0) {
        if (header->last->free) {
            MergeLastToThis(header);
            return;
        }
    }

    if (header->next != 0) {
        if (header->next->free) {
            MergeThisToNext(header);
            return;
        }
    }
}

void* _realloc(struct page_directory* pml4, struct heap * cheap, void* buffer, uint64_t size) {
    if (!cheap->ready) return 0;
    void* newBuffer = _malloc(pml4, cheap, size);
    if (newBuffer == 0) return 0;
    if (buffer == 0) return newBuffer;
    uint64_t oldSize = GetHeapSegmentHeader(buffer)->length; //TODO: Check if this is correct
    if (size < oldSize) oldSize = size;

    memcpy(newBuffer, buffer, oldSize);
    _free(cheap, buffer);

    return newBuffer;
}

void expand_heap(struct page_directory* pml4, struct heap * cheap, uint64_t length) {
    length += sizeof(struct heap_segment_header);
    if (length % PAGE_SIZE) {
        length -= length % PAGE_SIZE;
        length += PAGE_SIZE;
    }

    //Divide rounded up length by page size
    uint64_t pages = length / PAGE_SIZE;
    if (length % PAGE_SIZE) pages++;

    uint8_t perms = PAGE_WRITE_BIT;
    if (!cheap->isKernel) perms |= PAGE_USER_BIT;

    struct page_directory* pd = get_pml4();

    for (uint64_t i = 0; i < pages; i++) {
        void * new_page = pmm_alloc(0x1000);

        cheap->heapEnd = (void*)((uint64_t)cheap->heapEnd - (uint64_t)PAGE_SIZE);
        map_current_memory(cheap->heapEnd, new_page, perms);
        if (pd != pml4) {
            map_memory(pml4, cheap->heapEnd, new_page, perms);
        }
        if (cheap->heapEnd == 0) {cheap->ready = 0; panic("Failed to expand heap");}
    }

    struct heap_segment_header* newSegment = (struct heap_segment_header*)cheap->heapEnd;

    if (cheap->lastSegment != 0 && cheap->lastSegment->free && cheap->lastSegment->last != 0) {
        uint64_t size = cheap->lastSegment->length + length;
        newSegment->length = size - sizeof(struct heap_segment_header);
        newSegment->free = 1;
        newSegment->isStack = 0;
        newSegment->signature = HEAP_SIGNATURE;
        newSegment->last = cheap->lastSegment->last;
        newSegment->last->next = newSegment;
        newSegment->next = 0;
        cheap->lastSegment = newSegment;
    } else {
        newSegment->length = length - sizeof(struct heap_segment_header);
        newSegment->free = 1;
        newSegment->isStack = 0;
        newSegment->signature = HEAP_SIGNATURE;
        newSegment->last = cheap->lastSegment;
        newSegment->next = 0;
        if (cheap->lastSegment != 0) cheap->lastSegment->next = newSegment;
        cheap->lastSegment = newSegment;
    }

    cheap->totalSize += (length + sizeof(struct heap_segment_header));
    cheap->freeSize += (length + sizeof(struct heap_segment_header));
}

void* _stackalloc(struct page_directory* pml4, struct heap * cheap, uint64_t length) {
    uint64_t pages = length / PAGE_SIZE;
    if (length % PAGE_SIZE) pages++;

    uint8_t perms = PAGE_WRITE_BIT;
    if (!cheap->isKernel) perms |= PAGE_USER_BIT;

    struct page_directory* pd = get_pml4();

    void * physical;
    for (uint64_t i = 0; i < pages; i++) {
        cheap->lastStack = (void*)((uint64_t)cheap->lastStack - (uint64_t)PAGE_SIZE);
        cheap->lastStack = request_current_page_at(cheap->lastStack, perms);
        if (cheap->lastStack == 0) panic("Out of memory");

        if (pd != pml4) {
            physical = virtual_to_physical(pd, cheap->lastStack);
            map_memory(pml4, cheap->lastStack, physical, perms);
        }
    }

    void * address = cheap->lastStack;	

    cheap->lastStack = (void*)((uint64_t)cheap->lastStack - (uint64_t)PAGE_SIZE);

    return address;
}

void init_heap() {
    void * kernelHeapStartAddress =  (void*)0xffffffff60000000;
    void * kernelHeapEndAddress =    (void*)0xffffffff6ffff000;
    void * kernelStackStartAddress = (void*)0xffffffff70000000;
    void * kernelStackEndAddress =   (void*)0xffffffff7ffff000;
    void * kernelSHeapStartAddress = (void*)0xffffffff80000000;
    void * kernelSHeapEndAddress =   (void*)0xffffffff8ffff000;
    void * kernelSStackStartAddress = (void*)0xffffffff90000000;
    void * kernelSStackEndAddress =   (void*)0xffffffff9ffff000;
    void * kernelSleepHeapStartAddress = (void*)0xffffffffa0000000;
    void * kernelSleepHeapEndAddress =   (void*)0xffffffffaffff000;
    void * kernelSleepStackStartAddress = (void*)0xffffffffb0000000;
    void * kernelSleepStackEndAddress =   (void*)0xffffffffbffff000;
    _initHeap(get_pml4(), &kernelGlobalHeap, kernelHeapStartAddress, kernelHeapEndAddress, kernelStackStartAddress, kernelStackEndAddress, 0);
    _initHeap(get_pml4(), &kernelSleepHeap, kernelSHeapStartAddress, kernelSHeapEndAddress, kernelSStackStartAddress, kernelSStackEndAddress, 0);
    _initHeap(get_pml4(), &kernelSignalHeap, kernelSleepHeapStartAddress, kernelSleepHeapEndAddress, kernelSleepStackStartAddress, kernelSleepStackEndAddress, 0);
}

void create_user_heap(process_t * task, struct heap * cheap) {
    void * userHeapStartAddress =    (void*)0x00007fff60000000;
    void * userHeapEndAddress =      (void*)0x00007fff6ffff000;
    void * userStackStartAddress =   (void*)0x00007fff70000000;
    void * userStackEndAddress =     (void*)0x00007fff7ffff000;
    
    _initHeap(task->vm, cheap, userHeapStartAddress, userHeapEndAddress, userStackStartAddress, userStackEndAddress, 1);
    task->heap = (void*)cheap;
}

void * smalloc(uint64_t size) {
    LOCK_HEAP(&kernelSleepHeap);
    void * result = _malloc(get_pml4(), &kernelSleepHeap, size);
    UNLOCK_HEAP(&kernelSleepHeap);
    return result;
}

void sfree(void* address) {
    LOCK_HEAP(&kernelSleepHeap);
    _free(&kernelSleepHeap, address);
    UNLOCK_HEAP(&kernelSleepHeap);
}

void * sigmalloc(uint64_t size) {
    LOCK_HEAP(&kernelSignalHeap);
    void * result = _malloc(get_pml4(), &kernelSignalHeap, size);
    UNLOCK_HEAP(&kernelSignalHeap);
    return result;
}

void sigfree(void* address) {
    LOCK_HEAP(&kernelSignalHeap);
    _free(&kernelSignalHeap, address);
    UNLOCK_HEAP(&kernelSignalHeap);
}

void * kmalloc(uint64_t size) {
    LOCK_HEAP(&kernelGlobalHeap);
    void * result = _malloc(get_pml4(), &kernelGlobalHeap, size);
    UNLOCK_HEAP(&kernelGlobalHeap);
    return result;
}

void kfree(void* address) {
     //TODO: Implement unmap on free
     LOCK_HEAP(&kernelGlobalHeap);
    _free(&kernelGlobalHeap, address);
    UNLOCK_HEAP(&kernelGlobalHeap);
}

void * kcalloc(uint64_t num, uint64_t size) {
    LOCK_HEAP(&kernelGlobalHeap);
    void * result = _calloc(get_pml4(), &kernelGlobalHeap, num, size);
    UNLOCK_HEAP(&kernelGlobalHeap);
    return result;
}

void * krealloc(void* buffer, uint64_t size) {
    LOCK_HEAP(&kernelGlobalHeap);
    void * result = _realloc(get_pml4(), &kernelGlobalHeap, buffer, size);
    UNLOCK_HEAP(&kernelGlobalHeap);
    return result;
}

void * kstackalloc(uint64_t length) {
    LOCK_STACK(&kernelGlobalHeap);
    void * result = _stackalloc(get_pml4(), &kernelGlobalHeap, length);
    UNLOCK_STACK(&kernelGlobalHeap);
    return result;
}

void * umalloc(process_t* task, uint64_t size) {
    LOCK_HEAP((struct heap*)task->heap);
    void * result = _malloc(task->vm, (struct heap*)task->heap, size);
    UNLOCK_HEAP((struct heap*)task->heap);
    return result;
}

void ufree(process_t* task, void* address) {
    LOCK_HEAP((struct heap*)task->heap);
    _free((struct heap*)task->heap, address);
    UNLOCK_HEAP((struct heap*)task->heap);
}

void * ucalloc(process_t* task, uint64_t num, uint64_t size) {
    LOCK_HEAP((struct heap*)task->heap);
    void * result = _calloc(task->vm, (struct heap*)task->heap, num, size);
    UNLOCK_HEAP((struct heap*)task->heap);
    return result;
}

void * urealloc(process_t* task, void* buffer, uint64_t size) {
    LOCK_HEAP((struct heap*)task->heap);
    void * result = _realloc(task->vm, (struct heap*)task->heap, buffer, size);
    UNLOCK_HEAP((struct heap*)task->heap);
    return result;
}

void * ustackalloc(process_t* task, uint64_t length) {
    LOCK_STACK((struct heap*)task->heap);
    void * result = _stackalloc(task->vm, (struct heap*)task->heap, length);
    UNLOCK_STACK((struct heap*)task->heap);
    return result;
}
