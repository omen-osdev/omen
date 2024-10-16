#include <omen/libraries/std/stdint.h>
#include <omen/apps/debug/debug.h>
#include <omen/managers/mem/pmm.h>
#include <omen/managers/mem/vmm.h>
#include <omen/apps/panic/panic.h>
#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <generic/config.h>

#define CACHE_BIT_SET(x)((x & PAGE_CACHE_DISABLE) >> 3)
#define NX_BIT_SET(x)((x & PAGE_NX_BIT) >> 2)
#define USER_BIT_SET(x)((x & PAGE_USER_BIT) >> 1)
#define WRITE_BIT_SET(x)(x & PAGE_WRITE_BIT)

void address_to_map(uint64_t address, struct page_map_index* map) {
    address >>= 12;
    map->P_i = address & 0x1ff;
    address >>= 9;
    map->PT_i = address & 0x1ff;
    address >>= 9;
    map->PD_i = address & 0x1ff;
    address >>= 9;    
    map->PDP_i = address & 0x1ff;
}

void * virtual_to_physical(struct page_directory * pml4, void* virtual) {
    struct page_map_index map;
    address_to_map((uint64_t)virtual, &map);
    struct page_directory_entry pde;

    pde = pml4->entries[map.PDP_i];
    struct page_directory* pdp;
    if (!pde.present) {
        return NULL;
    } else {
        pdp = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    }

    pde = pdp->entries[map.PD_i];
    struct page_directory* pd;
    if (!pde.present) {
        return NULL;
    } else {
        pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    }

    pde = pd->entries[map.PT_i];
    struct page_table* pt;
    if (!pde.present) {
        return NULL;
    } else {
        pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
    }

    struct page_table_entry pte = pt->entries[map.P_i];
    if (!pte.present) {
        return NULL;
    }

    return (void*)((uint64_t)pte.page_ppn << 12);
}

struct page_directory* get_pml4() {
    struct page_directory* pml4;
    __asm__("movq %%cr3, %0" : "=r"(pml4));
    return pml4;
}

void set_pml4(struct page_directory* pml4) {
    __asm__("movq %0, %%cr3" : : "r" (pml4));
}

void invalidate_current_pml4() {
    struct page_directory* pml4 = get_pml4();
    set_pml4(pml4);
}

void debug_directory(struct page_directory* pd, uint8_t levels) {
    for (uint64_t i = 0; i < 512; i++) {
        struct page_directory_entry pde = pd->entries[i];
        if (pde.present) {
            kprintf("[%d] PD Entry: %llx BITS (W:%d U:%d NX:%d C:%d)\n", i, pde.page_ppn, pde.writeable, pde.user_access, pde.execution_disabled, pde.cache_disabled);
            if (levels >= 2) {
                struct page_directory* pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
                for (uint64_t j = 0; j < 512; j++) {
                    struct page_directory_entry pde = pd->entries[j];
                    if (pde.present) {
                        kprintf("    [%d] PT Entry: %llx BITS (W:%d U:%d NX:%d C:%d)\n", j, pde.page_ppn, pde.writeable, pde.user_access, pde.execution_disabled, pde.cache_disabled);
                        if (levels >= 3) {
                            struct page_table* pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
                            for (uint64_t k = 0; k < 512; k++) {
                                struct page_table_entry pte = pt->entries[k];
                                if (pte.present) {
                                    kprintf("        [%d] P Entry: %llx (Virt: %llx -> Phys: %llx) BITS (W:%d U:%d NX:%d C:%d)\n", k, pte.page_ppn, (i << 39) | (j << 30) | (k << 21), (pte.page_ppn << 12), pte.writeable, pte.user_access, pte.execution_disabled, pte.cache_disabled);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void debug_compare_maps(struct page_directory* pd, struct page_directory* pd2) {
    if (memcmp(pd, pd2, PAGE_SIZE)) {
        kprintf("ERROR: Page directories do not match\n");
        debug_directory(pd, 2);
        panic("ERROR: Page directories do not match");
    }
    for (uint64_t i = 0; i < 512; i++) {
        struct page_directory_entry pde = pd->entries[i];
        struct page_directory_entry pde2 = pd2->entries[i];
        if (pde.present) {
            struct page_directory* pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
            struct page_directory* pd2 = (struct page_directory*)((uint64_t)pde2.page_ppn << 12);
            if (memcmp(pd, pd2, PAGE_SIZE)) {
                kprintf("ERROR: Page directories do not match\n");
                debug_directory(pd, 2);
                panic("ERROR: Page directories do not match");
            }
            for (uint64_t j = 0; j < 512; j++) {
                struct page_directory_entry pde = pd->entries[j];
                struct page_directory_entry pde2 = pd2->entries[j];
                if (pde.present) {
                    struct page_table* pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
                    struct page_table* pt2 = (struct page_table*)((uint64_t)pde2.page_ppn << 12);
                    if (memcmp(pt, pt2, PAGE_SIZE)) {
                        kprintf("ERROR: Page tables do not match\n");
                        debug_directory(pd, 3);
                        panic("ERROR: Page tables do not match");
                    }
                    for (uint64_t k = 0; k < 512; k++) {
                        struct page_table_entry pte = pt->entries[k];
                        struct page_table_entry pte2 = pt2->entries[k];
                        if (pte.present) {
                            if (memcmp(&pte, &pte2, sizeof(struct page_table_entry))) {
                                kprintf("ERROR: Page table entries do not match\n");
                                kprintf("    [%d] P Entry: %llx (Virt: %llx -> Phys: %llx) BITS (W:%d U:%d NX:%d C:%d)\n", k, pte.page_ppn, (i << 39) | (j << 30) | (k << 21), (pte.page_ppn << 12), pte.writeable, pte.user_access, pte.execution_disabled, pte.cache_disabled);
                                panic("ERROR: Page table entries do not match");
                            }
                        }
                    }
                }
            }
        }
    }
}

void init_paging() {
    struct page_directory * pml4 = (struct page_directory*)pmm_alloc(PAGE_SIZE);
    if (pml4 == NULL) {
        panic("ERROR: Could not allocate page for PML4\n");
    }
    memset(pml4, 0, PAGE_SIZE);

    struct page_directory * original;
    __asm__("movq %%cr3, %0" : "=r"(original));

    kprintf("Original PML4: %llx\n", original);
    debug_directory(original, 2);
    duplicate_pml4(original, pml4, 0);
    debug_directory(pml4, 2);
    __asm__("movq %0, %%cr3" : : "r" (pml4));

    uint64_t virtual_start = get_kernel_address_virtual();
    uint64_t linker_kstart = (uint64_t)&KERNEL_START;

    if (linker_kstart != virtual_start) {
        kprintf("Crashing: KERNEL_START: %llx VIRT_ADDR: %p\n", linker_kstart, virtual_start);
        panic("init_paging: kernel virtual address does not match KERNEL_START");
    }

    kprintf("Paging initialized...\n");

}

struct page_directory* get_entry(struct page_directory* pd, uint64_t index) {
    struct page_directory_entry * pde = &(pd->entries[index]);
    if (pde && !(pde->present)) {

        struct page_directory* new_pd = (struct page_directory*)pmm_alloc(PAGE_SIZE);
        if (new_pd == NULL) {
            panic("ERROR: Could not allocate page for new PD\n");
        }

        memset(new_pd, 0, PAGE_SIZE);
        pde->page_ppn = (uint64_t)new_pd >> 12;
        pde->present = 1;
        pde->writeable = 1;
        pde->user_access = 1;
    }

    return (struct page_directory*)((uint64_t)pde->page_ppn << 12);
}

void map_memory(struct page_directory* pml4, void* virtual_memory, void* physical_memory, uint8_t flags) {

    if ((uint64_t)virtual_memory & 0xfff) {
        kprintf("Crashing: virtual_memory: %p\n", virtual_memory);
        panic("map_memory: virtual_memory must be aligned to 0x1000");
    }

    if ((uint64_t)physical_memory & 0xfff) {
        kprintf("Crashing: physical_memory: %p\n", physical_memory);
        panic("map_memory: physical_memory must be aligned to 0x1000");
    }

    struct page_map_index map;
    address_to_map((uint64_t)virtual_memory, &map);

    struct page_directory * pdp = get_entry(pml4, map.PDP_i);
    struct page_directory * pd = get_entry(pdp, map.PD_i);
    struct page_directory * pt = get_entry(pd, map.PT_i);

    struct page_table_entry * pte = (struct page_table_entry *)&(pt->entries[map.P_i]);
    pte->present = 1;
    pte->writeable = WRITE_BIT_SET(flags);
    pte->user_access = USER_BIT_SET(flags);
    pte->execution_disabled = NX_BIT_SET(flags);
    pte->cache_disabled = CACHE_BIT_SET(flags);
    pte->page_ppn = (uint64_t)physical_memory >> 12;

    __asm__("invlpg %0" : : "m" (*(char*)virtual_memory));
}

void unmap_memory(struct page_directory* pml4, void* virtual_address) {
    struct page_map_index map;
    address_to_map((uint64_t)virtual_address, &map);

    struct page_directory_entry pde;
    struct page_directory *pd;

    pde = pml4->entries[map.PDP_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PD_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PT_i];
    
    struct page_table *pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
    struct page_table_entry *pte = &pt->entries[map.P_i];

    pte->present = 0;
    __asm__("invlpg %0" : : "m" (*(char*)virtual_address));
}

void map_current_memory(void* virtual_memory, void* physical_memory, uint8_t flags) {
    struct page_directory* pml4 = get_pml4();
    map_memory(pml4, virtual_memory, physical_memory, flags);
}

void unmap_current_memory(void* virtual_address) {
    struct page_directory* pml4 = get_pml4();
    unmap_memory(pml4, virtual_address);
}

void set_page_perms(struct page_directory *pml4, void* address, uint8_t permissions) {
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory_entry pde;
    struct page_directory *pd;

    pde = pml4->entries[map.PDP_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PD_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PT_i];
    
    struct page_table *pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
    struct page_table_entry *pte = &pt->entries[map.P_i];

    pte->writeable = (permissions & 1);
    pte->user_access = ((permissions & 2) >> 1);
    pte->execution_disabled = ((permissions & 4) >> 2);
    pte->cache_disabled = ((permissions & 8) >> 3);
}

uint8_t get_page_perms(struct page_directory *pml4, void* address) {
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory_entry pde;
    struct page_directory *pd;

    pde = pml4->entries[map.PDP_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PD_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PT_i];
    
    struct page_table *pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
    struct page_table_entry pte = pt->entries[map.P_i];

    uint8_t result = pte.writeable;
    result |= (pte.user_access << 1);
    result |= (pte.execution_disabled << 2);
    result |= (pte.cache_disabled << 3);

    return result;
}

uint8_t get_os_bits(struct page_directory *pml4, void* address) {
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory_entry pde;
    struct page_directory *pd;

    pde = pml4->entries[map.PDP_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PD_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PT_i];
    
    struct page_table *pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
    struct page_table_entry pte = pt->entries[map.P_i];

    return pte.os;
}

void set_os_bits(struct page_directory *pml4, void* address, uint8_t bits) {
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory_entry pde;
    struct page_directory *pd;

    pde = pml4->entries[map.PDP_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PD_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PT_i];
    
    struct page_table *pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
    struct page_table_entry *pte = &pt->entries[map.P_i];

    pte->os = bits;
}

void mprotect(struct page_directory *pml4, void* address, uint64_t size, uint8_t permissions) {
    uint64_t start = (uint64_t)address;
    uint64_t end = start + size;
    start = start & ~0xfff;
    end = (end + 0xfff) & ~0xfff;

    for (uint64_t i = start; i < end; i += 0x1000) {
        set_page_perms(pml4, (void*)i, permissions);
    }
}

uint8_t remap_allocate_cow(struct page_directory * pml4, void * address_raw) {

    void * address = (void*)((uint64_t)address_raw & 0xFFFFFFFFFFFFF000);
    uint8_t current_perms = get_page_perms(pml4, address);
    uint8_t os_bits = get_os_bits(pml4, address);

    if (current_perms & 1 || !(os_bits & 1)) {
        return 0;
    }

    void * new_page = pmm_alloc(PAGE_SIZE);
    if (new_page == NULL) {
        panic("ERROR: Could not allocate page for remap\n");
    }

    //Map the new page to cow_working_page
    map_memory(pml4, COW_WORKING_PAGE, new_page, PAGE_WRITE_BIT);
    //Copy the data from the old page to the new page
    memcpy(COW_WORKING_PAGE, address, PAGE_SIZE);
    //Map the new page to the address
    map_memory(pml4, address, new_page, current_perms | PAGE_WRITE_BIT);
    //Remove the old page from the cow list
    set_os_bits(pml4, address, 0);

    return 1;
}

void duplicate_pml4(struct page_directory * pd, struct page_directory* new_pd, uint8_t use_cow) {
    memcpy(new_pd, pd, PAGE_SIZE);
    for (uint64_t i = 0; i < 512; i++) {
        struct page_directory_entry pde = pd->entries[i];
        if (pde.present) {
            struct page_directory* pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
            struct page_directory* new_pd = (struct page_directory*)pmm_alloc(PAGE_SIZE);
            memcpy(new_pd, pd, PAGE_SIZE);
            for (uint64_t j = 0; j < 512; j++) {
                struct page_directory_entry pde = pd->entries[j];
                if (pde.present) {
                    struct page_table* pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
                    struct page_table* new_pt = (struct page_table*)pmm_alloc(PAGE_SIZE);
                    memcpy(new_pt, pt, PAGE_SIZE);
                    for (uint64_t k = 0; k < 512; k++) {
                        struct page_table_entry pte = pt->entries[k];
                        if (pte.present) {
                            struct page_table_entry* new_pte = &new_pt->entries[k];
                            new_pte->present = 1;
                            new_pte->writeable = pte.writeable;
                            new_pte->user_access = pte.user_access;
                            new_pte->execution_disabled = pte.execution_disabled;
                            new_pte->cache_disabled = pte.cache_disabled;
                            new_pte->page_ppn = pte.page_ppn;
                            if (use_cow && new_pte->writeable && new_pte->user_access) {
                                new_pte->os = 1;
                                new_pte->writeable = 0;
                                pte.os = 1;
                                pte.writeable = 0;
                            }
                        }
                    }
                }
            }
        }
    }

    if (use_cow) return;
    debug_compare_maps(pd, new_pd);
}

void duplicate_current_pml4(struct page_directory* new_pml4) {
    struct page_directory* pml4 = get_pml4();
    duplicate_pml4(pml4, new_pml4, 0);
}

void mprotect_current(void* address, uint64_t size, uint8_t permissions) {
    struct page_directory* pml4 = get_pml4();
    mprotect(pml4, address, size, permissions);
}

uint8_t is_present(struct page_directory* pml4, void * address) {
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory_entry pde;
    struct page_directory *pd;

    pde = pml4->entries[map.PDP_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PD_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PT_i];
    
    struct page_table *pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
    struct page_table_entry pte = pt->entries[map.P_i];
    kprintf("pte.present: %d\n", pte.present);
    return pte.present;
}

uint8_t is_user_access(struct page_directory* pml4, void * address) {
//Check the user bit for each level of directory also
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory_entry pde;
    struct page_directory *pd;

    pde = pml4->entries[map.PDP_i];
    if (!pde.user_access) {
        kprintf("PDP not user accessible\n");
        return 0;
    }
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PD_i];
    if (!pde.user_access) {
        kprintf("PD not user accessible\n");
        return 0;
    }
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PT_i];
    if (!pde.user_access) {
        kprintf("PT not user accessible\n");
        return 0;
    }

    struct page_table *pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
    struct page_table_entry pte = pt->entries[map.P_i];
    kprintf("pte.user_access: %d\n", pte.user_access);
    return pte.user_access;
}

uint8_t is_executable(struct page_directory* pml4, void * address) {
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory_entry pde;
    struct page_directory *pd;

    pde = pml4->entries[map.PDP_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PD_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PT_i];
    
    struct page_table *pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
    struct page_table_entry pte = pt->entries[map.P_i];
    kprintf("pte.execution_disabled: %d\n", pte.execution_disabled);
    return !pte.execution_disabled;
}

void * request_current_page_at(void* vaddr, uint8_t flags) {
    struct page_directory *pml4 = get_pml4();
    void * result = pmm_alloc(0x1000);
    if (result == NULL) {
        panic("ERROR: Could not allocate page for mapping\n");
    }
    map_memory(pml4, vaddr, result, flags);
    return vaddr;
}