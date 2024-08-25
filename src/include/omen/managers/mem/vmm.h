#ifndef _VMM_H
#define _VMM_H
#include <omen/libraries/std/stdint.h>
#include <omen/managers/mem/pmm.h>
#include <omen/hal/arch/x86/vm.h>

pdTable* vmm_get_pml4();
void vmm_set_pml4(pdTable* pml4);
void vmm_page_to_map(uint64_t virt_addr, pageMapIndex *map);
void* vmm_virt_to_phys(uint64_t virt_addr, pdTable *pml4);
void vmm_init();

void *vmm_map_current(void* virt_addr, void* physical_addr, uint8_t flags);
void vmm_unmap_current(void* virt_addr);

#endif