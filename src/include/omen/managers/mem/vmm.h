#ifndef _VMM_H
#define _VMM_H
#include <omen/libraries/std/stdint.h>
#include <omen/managers/mem/pmm.h>
#include <omen/hal/arch/x86/vm.h>



void init_paging();
struct page_directory* get_pml4();
void invalidate_current_pml4();
void set_pml4(struct page_directory* pml4);
uint8_t get_page_perms(struct page_directory *pml4, void* address);
void * virtual_to_physical(struct page_directory *, void*);
void map_current_memory(void*, void*, uint8_t);
void map_memory(struct page_directory *, void*, void*, uint8_t);
void unmap_current_memory(void*);	
void unmap_memory(struct page_directory *, void*);
void mprotect(struct page_directory *, void*, uint64_t, uint8_t);
void mprotect_current(void*, uint64_t, uint8_t);
uint8_t is_present(struct page_directory* pml4, void * address);
uint8_t is_user_access(struct page_directory* pml4, void * address);
uint8_t is_executable(struct page_directory* pml4, void * address);
void * request_current_page_at(void* vaddr, uint8_t flags);
#endif