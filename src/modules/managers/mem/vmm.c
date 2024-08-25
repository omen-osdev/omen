#include <omen/managers/mem/vmm/paging_structs.h>
#include <omen/managers/mem/pmm.h>
#include <omen/libraries/std/stdint.h>

void page_to_map(uint64_t virt_addr, pageMapIndex *map) {
    virt_addr >>= 12;
    map->PT_idx = virt_addr & 0x1ff;
    virt_addr >>= 9;
    map->PD_idx = virt_addr & 0x1ff;
    virt_addr >>= 9;
    map->PDPT_idx = virt_addr & 0x1ff;
    virt_addr >>= 9;
    map->PML4_idx = virt_addr & 0x1ff;
}

void* virt_to_phys(uint64_t virt_addr, pdTable *pml4) {
    pageMapIndex idxMap;
    page_to_map(virt_addr, &idxMap);

    pml4_entry pml4_e =  pml4->entry[idxMap.PML4_idx];
    pdTable *pdpt = NULL;
    if((pml4_e & PDE_PRESENT) == 0b01) {
        pdpt = (pdptTable*)pml4_e; // check
    }
    else {
        return NULL;
    }

    pdpt_entry pdpt_e = pdpt->entry[idxMap.PDPT_idx];
    pdTable *pd = NULL;
    if( (pdpt_e & PDE_PRESENT) == 0b01) {
        pd = (pdTable*)pdpt_e;
    }
    else {
        return NULL;
    }

    pd_entry pd_e = pd->entry[idxMap.PD_idx];
    ptTable *pt = NULL;
    if( (pd_e & PDE_PRESENT) == 0b01) {
        pt = (ptTable*)pt->entry[idxMap.PT_idx];
    }
    else {
        return NULL;
    }

    pt_entry pt_e = pt->entry[idxMap.PT_idx];
    void *pagePtr = NULL;
    if( (pt_e & PTE_PRESENT) == 0b01) {
        return pagePtr;
    }
    else {
        return NULL;
    }
}

page_directory* get_pml4() {
    struct page_directory* pml4;
    __asm__("movq %%cr3, %0" : "=r"(pml4));
    return TO_KERNEL_MAP(pml4);
}

void set_pml4(struct page_directory* pml4) {
    __asm__("movq %0, %%cr3" : : "r" (FROM_KERNEL_MAP(pml4)));
}



