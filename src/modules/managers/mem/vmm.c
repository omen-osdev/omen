#include <omen/libraries/std/stdint.h>
#include <omen/managers/mem/pmm.h>
#include <omen/managers/mem/vmm.h>
#include <omen/apps/panic/panic.h>
#include <omen/libraries/std/string.h>
#include <generic/config.h>

pdTable * current_pml4 = NULL;


void vmm_page_to_map(uint64_t virt_addr, pageMapIndex *map) {
    virt_addr >>= 12;
    map->P_i = virt_addr & 0x1ff;
    virt_addr >>= 9;
    map->PT_i = virt_addr & 0x1ff;
    virt_addr >>= 9;
    map->PD_i = virt_addr & 0x1ff;
    virt_addr >>= 9;
    map->PDP_i = virt_addr & 0x1ff;
}

void* vmm_virt_to_phys(uint64_t virt_addr, pdTable *pml4) {

    if ((uint64_t)virt_addr & 0xfff) return NULL;
    pageMapIndex idxMap;
    vmm_page_to_map(virt_addr, &idxMap);

    pd_entry pml4_e = pml4->entry[idxMap.PDP_i];
    pdTable *pdpt = NULL;
    if((pml4_e.present) == 0b01) {
        pdpt = (pdTable*)(((uint64_t)pml4_e.page_ppn) << 12);
    }
    else {
        return NULL;
    }

    pd_entry pdpt_e = pdpt->entry[idxMap.PD_i];
    pdTable *pd = NULL;
    if((pdpt_e.present) == 0b01) {
        pd = (pdTable*)(((uint64_t)pdpt_e.page_ppn) << 12);
    }
    else {
        return NULL;
    }

    pd_entry pd_e = pd->entry[idxMap.PT_i];
    ptTable *pt = NULL;
    if( (pd_e.present) == 0b01) {
        pt = (ptTable*)(((uint64_t)pd_e.page_ppn) << 12);
    }
    else {
        return NULL;
    }

    pt_entry pt_e = pt->entry[idxMap.P_i];
    if( (pt_e.present) == 0b01) {
        return (void*)(((uint64_t)pt_e.page_ppn) << 12);
    }
    else {
        return NULL;
    }
}

void vmm_init() {
    current_pml4 = vmm_get_pml4();
}

pdTable* vmm_get_pml4() {
    pdTable * pml4;
    __asm__("movq %%cr3, %0" : "=r" (pml4));
    return pml4;
}

void vmm_set_pml4(pdTable* pml4) {
    __asm__("movq %0, %%cr3" : : "r" (pml4));
}

void* vmm_map_current(void* virt_addr, void* physical_addr, uint8_t flags) {
    if (current_pml4 == NULL) return NULL;
    if ((uint64_t)virt_addr & 0xfff) return NULL;
    if ((uint64_t)physical_addr & 0xfff) return NULL;

    pageMapIndex idxMap;
    vmm_page_to_map((uint64_t)virt_addr, &idxMap);
    
    pd_entry pd_e = current_pml4->entry[idxMap.PDP_i];
    pd_entry * pdp;
    if (!pd_e.present) {
        pdp = (pd_entry*) pmm_alloc(PAGE_SIZE);
        if (pdp == NULL) {
            panic("Can't allocate pdp");
        }
        memset(pdp, 0, PAGE_SIZE);
        pd_e.page_ppn = (((uint64_t)pdp) >> 12);
        pd_e.present = 1;
        pd_e.writeable = 1;
        pd_e.user_access = 1;
        current_pml4->entry[idxMap.PDP_i] = pd_e;
    } else {
        pdp = (pd_entry *)(((uint64_t)pd_e.page_ppn) << 12);
    }

    pd_e = ((pdTable*)pdp)->entry[idxMap.PD_i];
    pd_entry * pd;
    if (!pd_e.present) {
        pd = (pd_entry*) pmm_alloc(PAGE_SIZE);
        if (pd == NULL) {
            panic("Can't allocate pd");
        }
        memset(pd, 0, PAGE_SIZE);
        pd_e.page_ppn = (((uint64_t)pd) >> 12);
        pd_e.present = 1;
        pd_e.writeable = 1;
        pd_e.user_access = 1;
        ((pdTable*)pdp)->entry[idxMap.PD_i] = pd_e;
    } else {
        pd = (pd_entry *)(((uint64_t)pd_e.page_ppn) << 12);
    }

    pd_e = ((pdTable*)pd)->entry[idxMap.PT_i];
    pt_entry * pt;
    if (!pd_e.present) {
        pt = (pt_entry*) pmm_alloc(PAGE_SIZE);
        if (pt == NULL) {
            panic("Can't allocate pt");
        }
        memset(pt, 0, PAGE_SIZE);
        pd_e.page_ppn = (((uint64_t)pt) >> 12);
        pd_e.present = 1;
        pd_e.writeable = 1;
        pd_e.user_access = 1;
        ((pdTable*)pd)->entry[idxMap.PT_i] = pd_e;
    } else {
        pt = (pt_entry *)(((uint64_t)pd_e.page_ppn) << 12);
    }

    pt_entry pt_e = ((ptTable*)pt)->entry[idxMap.P_i];
    pt_e.page_ppn = (((uint64_t)physical_addr) >> 12);
    pt_e.present = 1;
    pt_e.writeable = VMM_WRITE_BIT_SET(flags);
    pt_e.user_access = VMM_USER_BIT_SET(flags);
    pt_e.execute_disable = VMM_NX_BIT_SET(flags);
    pt_e.cache_disabled = VMM_CACHE_DISABLE_BIT_SET(flags);
    ((ptTable*)pt)->entry[idxMap.P_i] = pt_e;
}


//TODO: Unmap physical memory
void vmm_unmap_current(void* virt_addr) {
    if (current_pml4 == NULL) return;
    if ((uint64_t)virt_addr & 0xfff) return;

    pageMapIndex idxMap;
    vmm_page_to_map((uint64_t)virt_addr, &idxMap);

    pd_entry pd_e = current_pml4->entry[idxMap.PDP_i];
    pd_entry * pdp;
    if (!pd_e.present) {
        return;
    } else {
        pdp = (pd_entry *)(((uint64_t)pd_e.page_ppn) << 12);
    }

    pd_e = ((pdTable*)pdp)->entry[idxMap.PD_i];
    pd_entry * pd;
    if (!pd_e.present) {
        return;
    } else {
        pd = (pd_entry *)(((uint64_t)pd_e.page_ppn) << 12);
    }

    pd_e = ((pdTable*)pd)->entry[idxMap.PT_i];
    pt_entry * pt;
    if (!pd_e.present) {
        return;
    } else {
        pt = (pt_entry *)(((uint64_t)pd_e.page_ppn) << 12);
    }

    pt_entry pt_e = ((ptTable*)pt)->entry[idxMap.P_i];
    pt_e.page_ppn = 0;
    pt_e.present = 0;
    pt_e.writeable = 0;
    pt_e.user_access = 0;
    pt_e.execute_disable = 0;
    pt_e.cache_disabled = 0;
    ((ptTable*)pt)->entry[idxMap.P_i] = pt_e;
}