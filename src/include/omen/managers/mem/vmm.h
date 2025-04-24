#ifndef _VMM_H
#define _VMM_H
#include <omen/libraries/std/stdint.h>
#include <omen/managers/mem/pmm.h>
#include <omen/hal/arch/x86/vm.h>

#define PAGE_SIZE_1GIB      0x40000000
#define PAGE_SIZE_2MIB      0x200000
#define PAGE_SIZE_4KIB      0x1000
#define PAGE_SIZE_DIR       0x1

#define VMM_WRITE_BIT          0x1
#define VMM_USER_BIT           0x2
#define VMM_WRITE_THROUGH_BIT  0x4 
#define VMM_CACHE_DISABLE_BIT  0x8
#define VMM_GLOBAL_BIT         0x10
#define VMM_NX_BIT             0x20

#define VMM_WRITE_BIT_SET(flags) ((flags) & VMM_WRITE_BIT)
#define VMM_USER_BIT_SET(flags) ((flags) & VMM_USER_BIT)
#define VMM_CACHE_BIT_SET(flags) ((flags) & VMM_CACHE_DISABLE_BIT)
#define VMM_NX_BIT_SET(flags) ((flags) & VMM_NX_BIT)
#define VMM_WRITE_THROUGH_BIT_SET(flags) ((flags) & VMM_WRITE_THROUGH_BIT)
#define VMM_GLOBAL_BIT_SET(flags) ((flags) & VMM_GLOBAL_BIT)
#define VMM_CACHE_DISABLE_BIT_SET(flags) ((flags) & VMM_CACHE_DISABLE_BIT)

#define VMM_CANONICAL_UPPER_MIN 0xFFFF800000000000
#define VMM_REGION_K_STACK      0xFFFF900000000000
#define VMM_REGION_K_HEAP       0xFFFFA00000000000
#define VMM_REGION_K_IDENT      0xFFFFB00000000000
#define VMM_REGION_DEVICES      0xFFFFC00000000000
#define VMM_REGION_U_VDSO       0xFFFFD00000000000

#define VMM_CANONICAL_LOWER_MAX 0x00007FFFFFFFFFFF
#define VMM_REGION_U_HEAP       0x0000300000000000
#define VMM_REGION_U_STACK      0x0000400000000000
#define VMM_REGION_U_SHM_MMAP   0x0000500000000000
#define COW_WORKING_PAGE        0x00007fff8ffff000

#define VMM_REGION_SIZE         0x000000F000000000

#define VMM_TO_DEVICE_MEMORY(addr) ((uint64_t)(addr) + (uint64_t)VMM_REGION_DEVICES)
#define VMM_FROM_DEVICE_MEMORY(addr) ((uint64_t)(addr) - (uint64_t)VMM_REGION_DEVICES)
#define VMM_TO_KERNEL_MEMORY(addr) ((uint64_t)(addr) + (uint64_t)VMM_REGION_K_IDENT)
#define VMM_FROM_KERNEL_MEMORY(addr) ((uint64_t)(addr) - (uint64_t)VMM_REGION_K_IDENT)
#define VMM_TO_KERNEL_STACK(addr) ((uint64_t)(addr) + (uint64_t)VMM_REGION_K_STACK)
#define VMM_FROM_KERNEL_STACK(addr) ((uint64_t)(addr) - (uint64_t)VMM_REGION_K_STACK)
#define VMM_TO_KERNEL_HEAP(addr) ((uint64_t)(addr) + (uint64_t)VMM_REGION_K_HEAP)
#define VMM_FROM_KERNEL_HEAP(addr) ((uint64_t)(addr) - (uint64_t)VMM_REGION_K_HEAP)
#define VMM_TO_USER_STACK(addr) ((uint64_t)(addr) + (uint64_t)VMM_REGION_U_STACK)
#define VMM_FROM_USER_STACK(addr) ((uint64_t)(addr) - (uint64_t)VMM_REGION_U_STACK)
#define VMM_TO_USER_HEAP(addr) ((uint64_t)(addr) + (uint64_t)VMM_REGION_U_HEAP)
#define VMM_FROM_USER_HEAP(addr) ((uint64_t)(addr) - (uint64_t)VMM_REGION_U_HEAP)

#define TO_IDENTITY_MAP(addr) (uint64_t)(((uint64_t)addr) + (uint64_t)physical_memory_offset)
#define FROM_IDENTITY_MAP(addr) (uint64_t)(((uint64_t)addr) - (uint64_t)physical_memory_offset)

//Used multiple times
struct page_directory* get_pml4();

void remap_allocate_cow(struct page_directory * pml4, void * section_start, uint64_t size, uint64_t page_size, uint8_t flags);
void * vmm_create_kernel_stack(struct page_directory* stack_root, uint64_t stack_pages, uint8_t flags, uint64_t * stack_base);
void * vmm_copy_stack(struct page_directory* stack_root, void * stack_base, uint64_t stack_size, uint8_t flags);
struct page_directory * vmm_copy_kernel(struct page_directory* root);
uint64_t vmm_is_present(struct page_directory* root, void * vmm_address);
void vmm_unmap_userspace(struct page_directory* root);
void * get_current_physical_address(void * address);
void * to_identity_map(void * address);
void * from_identity_map(void * address);
void * allocate_at_vaddr(struct page_directory * root, void * virtual_address, uint64_t size, uint8_t flags);
struct page_directory * vmm_copy(struct page_directory* root);

void map_memory(struct page_directory * pml4, void * address, void * physical, uint64_t page_size, uint8_t flags);
void unmap_memory(struct page_directory* root, void* virtual_address);
void mprotect(struct page_directory *, void*, uint64_t, uint8_t);
void mprotect_current(void*, uint64_t, uint8_t);
uint8_t is_user_access(struct page_directory* pml4, void * address);

void map_range(struct page_directory* root, void * virtual_start, void * physical_start, uint64_t page_size, uint64_t size, uint8_t flags);
void unmap_range(struct page_directory* root, void * virtual_start, uint64_t size);

void * allocate_vmm(struct page_directory * pml4, uint64_t size, uint64_t region, uint8_t flags);
void free_vmm(struct page_directory * pml4, void * address);

void* get_physical_address(struct page_directory* root, void* virtual_address);
void switch_cr3(struct page_directory* cr3);
void * allocate_phys_page();
void free_phys_page(void * address);

//Used one time
void init_paging();


#endif