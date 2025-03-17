#include <omen/libraries/std/stdint.h>
#include <omen/apps/debug/debug.h>
#include <omen/managers/mem/pmm.h>
#include <omen/managers/mem/vmm.h>
#include <omen/apps/panic/panic.h>
#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/libraries/std/string.h>
#include <generic/config.h>

#define PAGE_SIZE_4K 0x1000
#define PAGE_SIZE_2M 0x200000
#define PAGE_SIZE_1G 0x40000000

#define CACHE_BIT_SET(x)((x & PAGE_CACHE_DISABLE_BIT) >> 3)
#define NX_BIT_SET(x)((x & PAGE_NX_BIT) >> 2)
#define USER_BIT_SET(x)((x & PAGE_USER_BIT) >> 1)
#define WRITE_BIT_SET(x)(x & PAGE_WRITE_BIT)
#define COW_BIT_SET(x)(x & PAGE_COW_BIT)

#define ALIGN_DOWN(addr, pagesize) (addr & ~(pagesize - 1))
#define ALIGN_UP(addr, pagesize) ((addr + (pagesize - 1)) & ~(pagesize - 1))
#define TO_PAGES(size, pagesize) ((size + pagesize - 1) / pagesize)
#define FROM_PAGES(pages, pagesize) (pages * pagesize)
#define IS_ALIGNED(addr, pagesize) ((addr & (pagesize - 1)) == 0)

#define GET_PDE(pd, index) (&(pd->entries[index]))
#define GET_PD(pde) ((struct page_directory*)((uint64_t)pde->page_ppn << 12))

#define ENTRY_CREATE 1
#define ENTRY_NO_CREATE 0

struct vaddr_info {
    struct page_map_index map;
    struct page_directory_entry * pml4e;
    struct page_directory_entry * pdpe;
    struct page_directory_entry * pde;
    struct page_directory_entry * pte;
    struct page_directory_entry * last_entry;
    void * virtual;
    void * physical;
    uint8_t present;
    uint8_t nx;
    uint8_t writeable;
    uint8_t user_access;
    uint8_t cache_disabled;
    uint8_t user_tree_access;
    uint8_t cow;
    uint64_t page_size;
    uint64_t page_first_address;
};
uint8_t fill_vaddr_info(struct page_directory* pml4, void * vaddr, struct vaddr_info * info);
struct page_directory * kernel_pml4 = 0;

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

struct page_directory* navigate_entry(struct page_directory_entry* pde, uint64_t index, uint8_t create) {
    if (!pde) panic("navigate_entry: pde is NULL");
    if (!(pde->present)) {
        if (!create) return NULL;
        struct page_directory* new_pd = (struct page_directory*)pmm_alloc(PAGE_SIZE);
        if (new_pd == NULL) {
            panic("ERROR: Could not allocate page for new PD\n");
        }

        memset(new_pd, 0, PAGE_SIZE);
        pde->page_ppn = (uint64_t)new_pd >> 12;
        pde->present = 1;
        pde->writeable = 1;
        pde->user_access = 1;

        __asm__("invlpg %0" : : "m" (*(char*)pde));
    }

    return GET_PD(pde);
}

void duplicate_entry(struct page_directory* target_pml4, struct page_directory * source_pml4, void * source_address) {
    struct vaddr_info source_info;
    if (fill_vaddr_info(source_pml4, source_address, &source_info) == 0) {
        panic("duplicate_entry: source address not found");
    }

    struct page_directory_entry * source_pde = source_info.last_entry;

    struct page_directory_entry * pml4e = GET_PDE(target_pml4, source_info.map.PDP_i);
    if (!pml4e) panic("duplicate_entry: pml4e is NULL");
    struct page_directory * pdp = navigate_entry(pml4e, source_info.map.PD_i, ENTRY_CREATE);
    if (!pdp) panic("duplicate_entry: pdp is NULL");
    struct page_directory_entry * pdpe = GET_PDE(pdp, source_info.map.PD_i);
    if (!pdpe) panic("duplicate_entry: pdpe is NULL");
    if (pdpe->size) {
        pdpe->page_ppn = source_pde->page_ppn;
        pdpe->present = source_pde->present;
        pdpe->writeable = source_pde->writeable;
        pdpe->user_access = source_pde->user_access;
        pdpe->execution_disabled = source_pde->execution_disabled;
        pdpe->cache_disabled = source_pde->cache_disabled;
        return;
    }

    struct page_directory * pd = navigate_entry(pdpe, source_info.map.PT_i, ENTRY_CREATE);
    if (!pd) panic("duplicate_entry: pd is NULL");
    struct page_directory_entry * pde = GET_PDE(pd, source_info.map.PT_i);
    if (!pde) panic("duplicate_entry: pde is NULL");
    if (pde->size) {
        pde->page_ppn = source_pde->page_ppn;
        pde->present = source_pde->present;
        pde->writeable = source_pde->writeable;
        pde->user_access = source_pde->user_access;
        pde->execution_disabled = source_pde->execution_disabled;
        pde->cache_disabled = source_pde->cache_disabled;
        return;
    }

    struct page_directory * pt = navigate_entry(pde, source_info.map.P_i, ENTRY_CREATE);
    if (!pt) panic("duplicate_entry: pt is NULL");
    struct page_directory_entry * pte = GET_PDE(pt, source_info.map.P_i);
    if (!pte) panic("duplicate_entry: pte is NULL");
    pte->page_ppn = source_pde->page_ppn;
    pte->present = source_pde->present;
    pte->writeable = source_pde->writeable;
    pte->user_access = source_pde->user_access;
    pte->execution_disabled = source_pde->execution_disabled;
    pte->cache_disabled = source_pde->cache_disabled;
}

void map_memory(struct page_directory * pml4, void * address, void * physical, uint64_t page_size, uint8_t flags) {
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);
    if (!IS_ALIGNED((uint64_t)address, page_size)) {
        panic("map_memory: address must be aligned to page size");
    }
    if (page_size != PAGE_SIZE_4K && page_size != PAGE_SIZE_2M && page_size != PAGE_SIZE_1G) {
        panic("map_memory: invalid page size");
    }

    struct page_directory_entry * pml4e = GET_PDE(pml4, map.PDP_i);
    if (!pml4e) panic("map_memory: pml4e is NULL");
    struct page_directory * pdp = navigate_entry(pml4e, map.PD_i, ENTRY_CREATE);
    if (!pdp) panic("map_memory: pdp is NULL");
    struct page_directory_entry * pdpe = GET_PDE(pdp, map.PD_i);
    if (!pdpe) panic("map_memory: pdpe is NULL");
    if (page_size == PAGE_SIZE_1G) {
        pdpe->size = 1;
        pdpe->page_ppn = (uint64_t)physical >> 30;
        pdpe->present = 1;
        pdpe->writeable = WRITE_BIT_SET(flags);
        pdpe->user_access = USER_BIT_SET(flags);
        pdpe->execution_disabled = NX_BIT_SET(flags);
        pdpe->cache_disabled = CACHE_BIT_SET(flags);

        __asm__("invlpg %0" : : "m" (*(char*)address));
        return;
    }

    struct page_directory * pd = navigate_entry(pdpe, map.PT_i, ENTRY_CREATE);
    if (!pd) panic("map_memory: pd is NULL");
    struct page_directory_entry * pde = GET_PDE(pd, map.PT_i);
    if (!pde) panic("map_memory: pde is NULL");
    if (page_size == PAGE_SIZE_2M) {
        pde->size = 1;
        pde->page_ppn = (uint64_t)physical >> 21;
        pde->present = 1;
        pde->writeable = WRITE_BIT_SET(flags);
        pde->user_access = USER_BIT_SET(flags);
        pde->execution_disabled = NX_BIT_SET(flags);
        pde->cache_disabled = CACHE_BIT_SET(flags);

        __asm__("invlpg %0" : : "m" (*(char*)address));
        return;
    }

    struct page_directory * pt = navigate_entry(pde, map.P_i, ENTRY_CREATE);
    if (!pt) panic("map_memory: pt is NULL");
    struct page_directory_entry * pte = GET_PDE(pt, map.P_i);
    if (!pte) panic("map_memory: pte is NULL");
    pte->present = 1;
    pte->writeable = WRITE_BIT_SET(flags);
    pte->user_access = USER_BIT_SET(flags);
    pte->execution_disabled = NX_BIT_SET(flags);
    pte->cache_disabled = CACHE_BIT_SET(flags);
    pte->page_ppn = (uint64_t)physical >> 12;

    __asm__("invlpg %0" : : "m" (*(char*)address));
    kprintf("Mapped %p to %p perms: %x\n", address, physical, flags);
}

void unmap_memory(struct page_directory * pml4, void * address) {
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);
    struct page_directory_entry * pml4e = GET_PDE(pml4, map.PDP_i);
    if (!pml4e) return;
    struct page_directory * pdp = navigate_entry(pml4e, map.PD_i, ENTRY_NO_CREATE);
    if (!pdp) return;
    struct page_directory_entry * pdpe = GET_PDE(pdp, map.PD_i);
    if (!pdpe) return;
    if (pdpe->size) {
        pdpe->present = 0;
        __asm__("invlpg %0" : : "m" (*(char*)address));
        return;
    }

    struct page_directory * pd = navigate_entry(pdpe, map.PT_i, ENTRY_NO_CREATE);
    if (!pd) return;
    struct page_directory_entry * pde = GET_PDE(pd, map.P_i);
    if (!pde) return;
    if (pde->size) {
        pde->present = 0;
        __asm__("invlpg %0" : : "m" (*(char*)address));
        return;
    }

    struct page_directory * pt = (struct page_directory *)navigate_entry(pde, map.P_i, ENTRY_NO_CREATE);
    if (!pt) return;
    struct page_directory_entry * pte = GET_PDE(pt, map.P_i);
    if (!pte) return;
    pte->present = 0;
    __asm__("invlpg %0" : : "m" (*(char*)address));
}

uint8_t flags_from_info(struct vaddr_info * info) {
    uint8_t flags = 0;
    if (info->writeable) flags |= PAGE_WRITE_BIT;
    if (info->user_access) flags |= PAGE_USER_BIT;
    if (info->nx) flags |= PAGE_NX_BIT;
    if (info->cache_disabled) flags |= PAGE_CACHE_DISABLE_BIT;
    return flags;
}

uint8_t fill_vaddr_info(struct page_directory* pml4, void * vaddr, struct vaddr_info * info) {
    memset(info, 0, sizeof(struct vaddr_info));
    address_to_map((uint64_t)vaddr, &info->map);
    struct page_directory_entry * pml4e = GET_PDE(pml4, info->map.PDP_i);
    if (!pml4e || !pml4e->present) return 0;
    struct page_directory * pdp = navigate_entry(pml4e, info->map.PD_i, ENTRY_NO_CREATE);
    if (!pdp) return 0;
    struct page_directory_entry * pdpe = GET_PDE(pdp, info->map.PD_i);
    if (!pdpe || !pdpe->present) return 0;
    if (pdpe->size) {
        //1GB page
        info->pml4e = pml4e;
        info->pdpe = pdpe;
        info->last_entry = pdpe;
        info->virtual = vaddr;
        info->physical = (void*)((uint64_t)pdpe->page_ppn << 30);
        info->present = 1;
        info->nx = pdpe->execution_disabled;
        info->writeable = pdpe->writeable;
        info->user_access = pdpe->user_access;
        info->user_tree_access = pdpe->user_access && pml4e->user_access;
        info->cache_disabled = pdpe->cache_disabled;
        info->cow = pdpe->cow;
        info->page_size = PAGE_SIZE_1G;
        info->page_first_address = (void*)((uint64_t)info->map.PDP_i << 39 | (uint64_t)info->map.PD_i << 30);
        return 1;
    }

    struct page_directory * pd = navigate_entry(pdpe, info->map.PT_i, ENTRY_NO_CREATE);
    if (!pd) return 0;
    struct page_directory_entry * pde = GET_PDE(pd, info->map.PT_i);
    if (!pde || !pde->present) return 0;
    if (pde->size) {
        //2MB page
        info->pml4e = pml4e;
        info->pdpe = pdpe;
        info->pde = pde;
        info->last_entry = pde;
        info->virtual = vaddr;
        info->physical = (void*)((uint64_t)pde->page_ppn << 21);
        info->present = 1;
        info->nx = pde->execution_disabled;
        info->writeable = pde->writeable;
        info->user_access = pde->user_access;
        info->user_tree_access = pde->user_access && pdpe->user_access && pml4e->user_access;
        info->cache_disabled = pde->cache_disabled;
        info->cow = pde->cow;
        info->page_size = PAGE_SIZE_2M;
        info->page_first_address = (void*)((uint64_t)info->map.PDP_i << 39 | (uint64_t)info->map.PD_i << 30 | (uint64_t)info->map.PT_i << 21);
        return 1;
    }

    struct page_directory * pt = navigate_entry(pde, info->map.P_i, ENTRY_NO_CREATE);
    if (!pt) return 0;
    struct page_directory_entry * pte = GET_PDE(pt, info->map.P_i);
    if (!pte || !pte->present) return 0;
    //4KB page
    info->pml4e = pml4e;
    info->pdpe = pdpe;
    info->pde = pde;
    info->pte = pte;
    info->last_entry = pte;
    info->virtual = vaddr;
    info->physical = (void*)((uint64_t)pte->page_ppn << 12);
    info->present = 1;
    info->nx = pte->execution_disabled;
    info->writeable = pte->writeable;
    info->user_access = pte->user_access;
    info->user_tree_access = pte->user_access && pde->user_access && pdpe->user_access && pml4e->user_access;
    info->cache_disabled = pte->cache_disabled;
    info->cow = pte->cow;
    info->page_size = PAGE_SIZE_4K;
    info->page_first_address = (void*)((uint64_t)info->map.PDP_i << 39 | (uint64_t)info->map.PD_i << 30 | (uint64_t)info->map.PT_i << 21 | (uint64_t)info->map.P_i << 12);
    return 1;
}

uint8_t remap_allocate_cow(struct page_directory * pml4, void * address_raw) {
    struct vaddr_info info;
    if (!fill_vaddr_info(pml4, address_raw, &info)) return 0;

    if (info.writeable || !info.cow) {
        return 0;
    }
    
    void * new_page = pmm_alloc(info.page_size);
    if (new_page == NULL) {
        panic("ERROR: Could not allocate page for remap\n");
    }

    map_memory(pml4, COW_WORKING_PAGE, new_page, info.page_size, flags_from_info(&info));
    memcpy(COW_WORKING_PAGE, info.page_first_address, info.page_size);
    map_memory(pml4, info.page_first_address, new_page, info.page_size, flags_from_info(&info) | PAGE_WRITE_BIT);
    
    info.last_entry->cow = 0;

    return 1;
}


void duplicate_page(struct page_directory_entry * original, struct page_directory_entry * new, uint64_t size) {
    void * new_buffer = pmm_alloc(size);
    if (new_buffer == NULL) {
        panic("ERROR: Could not allocate page for new buffer\n");
    }

    switch (size) {
        case PAGE_SIZE_1G: {
            memcpy(new_buffer, (void*)((uint64_t)original->page_ppn << 30), size);
            new->page_ppn = (uint64_t)new_buffer >> 30;
            break;
        }
        case PAGE_SIZE_2M: {
            memcpy(new_buffer, (void*)((uint64_t)original->page_ppn << 21), size);
            new->page_ppn = (uint64_t)new_buffer >> 21;
            break;
        }
        case PAGE_SIZE_4K: {
            memcpy(new_buffer, (void*)((uint64_t)original->page_ppn << 12), size);
            new->page_ppn = (uint64_t)new_buffer >> 12;
            break;
        }
    }
}

struct page_directory * duplicate_pd(struct page_directory * pml4, uint8_t share_kernel, uint8_t use_cow) {
    struct page_directory * new_pml4 = (struct page_directory*)pmm_alloc(PAGE_SIZE);
    if (new_pml4 == NULL) {
        panic("ERROR: Could not allocate page for new PML4\n");
    }

    memset(new_pml4, 0, PAGE_SIZE);
    uint64_t max = 512;
    if (share_kernel) {
        max = 256;
        for (uint64_t i = 256; i < 512; i++)
            new_pml4->entries[i] = pml4->entries[i];
    }
    for (uint64_t i = 0; i < max; i++) {
        struct page_directory_entry * pml4e = GET_PDE(pml4, i);
        struct page_directory_entry * new_pml4e = GET_PDE(new_pml4, i);
        if (pml4e->present) {
            memcpy(new_pml4e, pml4e, sizeof(struct page_directory_entry));
            struct page_directory * pdp = GET_PD(pml4e);
            struct page_directory * new_pdp = (struct page_directory*)pmm_alloc(PAGE_SIZE);
            if (new_pdp == NULL) {
                panic("ERROR: Could not allocate page for new PDP\n");
            }

            memset(new_pdp, 0, PAGE_SIZE);
            new_pml4e->page_ppn = (uint64_t)new_pdp >> 12;
            for (uint64_t j = 0; j < 512; j++) {
                struct page_directory_entry * pdpe = GET_PDE(pdp, j);
                struct page_directory_entry * new_pdpe = GET_PDE(new_pdp, j);
                if (pdpe->present) {
                    memcpy(new_pdpe, pdpe, sizeof(struct page_directory_entry));
                    if (pdpe->size) {
                        if (share_kernel && !pdpe->user_access) continue;
                        if (use_cow) {
                            new_pdpe->cow = 1;
                            pdpe->cow = 1;
                            new_pdpe->writeable = 0;
                            pdpe->writeable = 0;
                        } else {
                            duplicate_page(pdpe, new_pdpe, PAGE_SIZE_1G);
                        }
                    }
                    struct page_directory * pd = GET_PD(pdpe);
                    struct page_directory * new_pd = (struct page_directory*)pmm_alloc(PAGE_SIZE);
                    if (new_pd == NULL) {
                        panic("ERROR: Could not allocate page for new PD\n");
                    }

                    memset(new_pd, 0, PAGE_SIZE);
                    new_pdpe->page_ppn = (uint64_t)new_pd >> 12;
                    for (uint64_t k = 0; k < 512; k++) {
                        struct page_directory_entry * pde = GET_PDE(pd, k);
                        struct page_directory_entry * new_pde = GET_PDE(new_pd, k);
                        if (pde->present) {
                            memcpy(new_pde, pde, sizeof(struct page_directory_entry));
                            if (pde->size) {
                                if (share_kernel && !pde->user_access) continue;
                                if (use_cow) {
                                    new_pde->cow = 1;
                                    pde->cow = 1;
                                    new_pde->writeable = 0;
                                    pde->writeable = 0;
                                } else {
                                    duplicate_page(pde, new_pde, PAGE_SIZE_2M);
                                }
                            }
                            struct page_directory * pt = GET_PD(pde);
                            struct page_directory * new_pt = (struct page_directory*)pmm_alloc(PAGE_SIZE);
                            if (new_pt == NULL) {
                                panic("ERROR: Could not allocate page for new PT\n");
                            }

                            memset(new_pt, 0, PAGE_SIZE);
                            new_pde->page_ppn = (uint64_t)new_pt >> 12;
                            for (uint64_t l = 0; l < 512; l++) {
                                struct page_directory_entry * pte = GET_PDE(pt, l);
                                struct page_directory_entry * new_pte = GET_PDE(new_pt, l);
                                if (pte->present) {
                                    memcpy(new_pte, pte, sizeof(struct page_directory_entry));
                                    if (use_cow) {
                                        new_pte->cow = 1;
                                        pte->cow = 1;
                                        new_pte->writeable = 0;
                                        pte->writeable = 0;
                                    } else {
                                        duplicate_page(pte, new_pte, PAGE_SIZE_4K);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return new_pml4;
}

void mprotect(struct page_directory * pml4, void* address_raw, uint64_t size, uint8_t permissions) {
    void * address = 0x0000ffffffffffff & (uint64_t)address_raw;
    //get pages in range address to address + size
    struct vaddr_info info;
    if (!fill_vaddr_info(pml4, address, &info)) panic("mprotect: Could not get page info");

    uint64_t page_first_address = (uint64_t)info.page_first_address;

    while (page_first_address < (uint64_t)address + size) {
        struct vaddr_info info;
        if (!fill_vaddr_info(pml4, (void*)page_first_address, &info)) panic("mprotect: Could not get page info");

        if (info.present) {
            if (USER_BIT_SET(permissions)) {
                if (info.pml4e) info.pml4e->user_access = 1;
                if (info.pdpe) info.pdpe->user_access = 1;
                if (info.pde) info.pde->user_access = 1;
                if (info.pte) info.pte->user_access = 1;
            }

            info.last_entry->writeable = WRITE_BIT_SET(permissions);
            info.last_entry->user_access = USER_BIT_SET(permissions);
            info.last_entry->execution_disabled = NX_BIT_SET(permissions);
            info.last_entry->cache_disabled = CACHE_BIT_SET(permissions);
            info.last_entry->cow = COW_BIT_SET(permissions);
        }

        page_first_address += info.page_size;
    }
}

void * virtual_to_physical(struct page_directory * pml4, void* address) {
    struct vaddr_info info;
    if (!fill_vaddr_info(pml4, address, &info)) panic("virtual_to_physical: Could not get page info");
    return info.physical;
}

uint8_t get_page_perms(struct page_directory *pml4, void* address) {
    struct vaddr_info info;
    if (!fill_vaddr_info(pml4, address, &info)) panic("get_page_perms: Could not get page info");
    return flags_from_info(&info);
}

uint8_t is_present(struct page_directory* pml4, void * address) {
    struct vaddr_info info;
    if (!fill_vaddr_info(pml4, address, &info)) panic("is_present: Could not get page info");
    return info.present;
}

uint8_t is_user_access(struct page_directory* pml4, void * address) {
    struct vaddr_info info;
    if (!fill_vaddr_info(pml4, address, &info)) panic("is_user_access: Could not get page info");
    return info.user_tree_access;
}

uint8_t is_writeable(struct page_directory* pml4, void * address) {
    struct vaddr_info info;
    if (!fill_vaddr_info(pml4, address, &info)) panic("is_writeable: Could not get page info");
    return info.writeable;
}

uint8_t is_executable(struct page_directory* pml4, void * address) {
    struct vaddr_info info;
    if (!fill_vaddr_info(pml4, address, &info)) panic("is_executable: Could not get page info");
    return !info.nx;
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

void set_kernel_pml4(struct page_directory* pml4) {
    kernel_pml4 = pml4;
}

struct page_directory* get_kernel_pml4() {
    return kernel_pml4;
}

void init_paging() {
    struct page_directory * pml4 = get_pml4();

    //uint64_t virtual_start = get_kernel_address_virtual();
    //uint64_t physical_start = get_kernel_address_physical();
    //struct pmm_block * memory = get_main_memory();

    struct page_directory* new_pml4 = duplicate_pd(pml4, 1, 0);

    set_pml4(new_pml4);
    set_kernel_pml4(new_pml4);

    kprintf("Paging initialized...\n");
}

struct page_directory * duplicate_current_pml4() {
    struct page_directory * pml4 = get_pml4();
    return duplicate_pd(pml4, 1, 0);
}

void unmap_current_memory(void* address) {
    struct page_directory * pml4 = get_pml4();
    unmap_memory(pml4, address);
}

void map_current_memory(void* address, void* physical, uint8_t flags) {
    struct page_directory * pml4 = get_pml4();
    map_memory(pml4, address, physical, PAGE_SIZE_4K, flags);
}

void mprotect_current(void* address, uint64_t size, uint8_t permissions) {
    struct page_directory * pml4 = get_pml4();
    mprotect(pml4, address, size, permissions);
}