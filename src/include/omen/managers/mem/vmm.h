#ifndef _VMM_H
#define _VMM_H
#include <omen/libraries/std/stdint.h>
#include <omen/managers/mem/pmm.h>
#include <omen/hal/arch/x86/vm.h>

#define VMM_WRITE_BIT           0x1
#define VMM_USER_BIT            0x2
#define VMM_NX_BIT              0x4
#define VMM_CACHE_DISABLE_BIT   0x8

#define COW_WORKING_PAGE 0x00007fff8ffff000

//Used multiple times
struct page_directory* get_pml4();
uint8_t remap_allocate_cow(struct page_directory * pml4, void * address_raw);
struct page_directory * duplicate_pd(struct page_directory * pml4, uint8_t share_kernel, uint8_t use_cow);
uint8_t get_page_perms(struct page_directory *pml4, void* address);
void map_memory(struct page_directory * pml4, void * address, void * physical, uint64_t page_size, uint8_t flags);
void mprotect(struct page_directory *, void*, uint64_t, uint8_t);
uint8_t is_user_access(struct page_directory* pml4, void * address);
struct page_directory* duplicate_current_pml4();
void debug_address(struct page_directory * pml4, void * address);
void * allocate_vmm_page(struct page_directory * pml4, uint8_t flags);
void free_vmm_page(struct page_directory * pml4, void * address);
void * allocate_vmm(struct page_directory * pml4, uint64_t size, uint8_t flags);
void free_vmm(struct page_directory * pml4, void * address);
void * allocate_current_vmm(uint64_t size, uint8_t flags);
void * free_current_vmm(void * address);

//Only internal use
void set_kernel_pml4(struct page_directory* pml4);
void set_pml4(struct page_directory* pml4);
void * virtual_to_physical(struct page_directory *, void*);
void duplicate_entry(struct page_directory* target_pml4, struct page_directory * source_pml4, void * source_address);
void unmap_memory(struct page_directory *, void*);
uint8_t is_present(struct page_directory* pml4, void * address);
uint8_t is_writeable(struct page_directory* pml4, void * address);
uint8_t is_user_access(struct page_directory* pml4, void * address);
uint8_t is_executable(struct page_directory* pml4, void * address);
//Unused
struct page_directory* get_kernel_pml4();
void invalidate_current_pml4();
//Used one time
void init_paging();


#endif