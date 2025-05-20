#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/managers/mem/vmm.h>
#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/libraries/concurrency/mutex.h>

void * kmalloc(uint64_t size) {
    void * ptr = allocate_vmm(get_pml4(), size, VMM_REGION_K_HEAP, 0x0);
    memset(ptr, 0, size);
    return ptr;
}

void _stkalloc(struct page_directory* pd, struct stack * stack, uint64_t length, uint64_t region) {
    uint64_t pages = length / 0x1000;

    if (length % 0x1000) {
        pages++;
    }

    uint64_t guard_size = STACK_GUARD_SIZE;
    if (guard_size % 0x1000) {
        guard_size += 0x1000 - (guard_size % 0x1000);
    }
    uint64_t guard_address = allocate_vmm(pd, (pages*0x1000)+guard_size, region, VMM_WRITE_BIT);
    if (guard_address == 0) {
        return NULL;
    }
    uint64_t base = guard_address + guard_size;
    if (region == VMM_REGION_K_STACK)
        memset((void*)to_identity_map(get_physical_address(pd, guard_address)), 0, (pages*0x1000)+guard_size);
    else if (region == VMM_REGION_U_STACK)
        memset((void*)to_identity_map(VMM_FROM_USER_STACK(guard_address)), 0, (pages*0x1000)+guard_size);
    else
        panic("Invalid region for stack allocation\n");
        //Deallocate the entire stack but keep physical memory
    uint64_t guard_phys = get_physical_address(pd, guard_address);
    uint64_t base_phys = get_physical_address(pd, base);
    if (guard_phys == 0 || base_phys == 0) {
        panic("Failed to get physical address\n");
    }

    unmap_range(pd, guard_address, (pages*0x1000)+guard_size);
    //Map the base range only
    //void map_range(struct page_directory* root, void * virtual_start, void * physical_start, uint64_t page_size, uint64_t size, uint8_t flags);
    map_range(pd, base, base_phys, 0x1000, pages*0x1000, VMM_WRITE_BIT | VMM_USER_BIT);

    uint64_t top = (uint64_t)(base+pages*0x1000)-0x10;
    //if unaligned_alloc is not 16-byte aligned, align it by subtracting the difference
    if (top % 0x10) {
        top -= top % 0x10;
    }
    top -= 0x8;
    
    stack->base = (void*)(base);
    stack->flags = 0;
    stack->top = (void*)(top);
    stack->size = (uint64_t)stack->top - (uint64_t)stack->base;
    stack->guard_size = guard_size;
}

void grow_stack(struct page_directory* pd, struct stack * stack) {
    stack->base = vmm_grow_stack(pd, stack->base, stack->size, stack->guard_size, stack->size + STACK_GROWTH_SIZE);
    stack->size += STACK_GROWTH_SIZE;
}

void kstackalloc(struct page_directory* pd, struct stack * stack, uint64_t length) {
    _stkalloc(pd, stack, length, VMM_REGION_K_STACK);
}
void stackalloc(struct page_directory* pd, struct stack * stack, uint64_t length) {
    _stkalloc(pd, stack, length, VMM_REGION_U_STACK);
}

void kstackfree(struct page_directory* pd, struct stack * stack) {
    unmap_range(pd, stack->base, stack->size);
    unmap_range(pd, stack->base - stack->guard_size, stack->guard_size);
}

void free(struct page_directory* root, void * address) {
    free_vmm(root, address);
}

void kfree(void* address) {
    free(get_pml4(), address); //In the future account for malloc size!!
}

void * malloc(struct page_directory* root, uint64_t size) {
    void * ptr = allocate_vmm(root, size, VMM_REGION_U_HEAP, VMM_WRITE_BIT);
    if (ptr == NULL) {
        return NULL;
    }
    memset(to_identity_map(VMM_FROM_USER_HEAP(ptr)), 0, size);
    return ptr;
}

void stackfree(struct page_directory* root, struct stack * stack) {
    unmap_range(root, stack->base, stack->size);
    unmap_range(root, stack->base - stack->guard_size, stack->guard_size);
}