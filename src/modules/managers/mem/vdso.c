#include <omen/managers/mem/vdso.h>
#include <omen/apps/panic/panic.h>
#include <omen/libraries/std/string.h>

void create_entry(vdso_t * vdso, uint8_t id) {
    vdso_entry_t * entry = (vdso_entry_t *)allocate_vmm(vdso->pd, sizeof(vdso_entry_t), VMM_REGION_U_VDSO, VMM_WRITE_BIT | VMM_USER_BIT | VMM_CACHE_DISABLE_BIT);
    if (entry == 0) {
        panic("Failed to allocate memory for VDSO entry\n");
        return;
    }
    uint64_t entry_physical = get_physical_address(vdso->pd, entry);
    if (entry_physical == 0) {
        panic("Failed to get physical address for VDSO entry\n");
        return;
    }
    map_range(get_pml4(), (uint64_t)entry, entry_physical, sizeof(vdso_entry_t), sizeof(vdso_entry_t), VMM_WRITE_BIT | VMM_USER_BIT | VMM_CACHE_DISABLE_BIT);
    memset(entry, 0, sizeof(vdso_entry_t));
    entry->id = id;
    entry->next = vdso->entry;
    vdso->entry = entry;
    entry->act = 0;
    entry->info = 0;
    entry->size = 0;
}

vdso_t* vdso_init(struct page_directory* pd) {
    vdso_t* vdso = (vdso_t*)allocate_vmm(pd, PAGE_SIZE_4KIB, VMM_REGION_U_VDSO, VMM_WRITE_BIT | VMM_USER_BIT | VMM_CACHE_DISABLE_BIT);
    if (vdso == 0) {
        panic("Failed to allocate memory for VDSO\n");
        return 0;
    }
    uint64_t vdso_physical = get_physical_address(pd, vdso);
    if (vdso_physical == 0) {
        panic("Failed to get physical address for VDSO\n");
        return 0;
    }
    map_range(get_pml4(), VMM_REGION_U_VDSO, vdso_physical, PAGE_SIZE_4KIB, PAGE_SIZE_4KIB, VMM_WRITE_BIT | VMM_USER_BIT | VMM_CACHE_DISABLE_BIT);
    memset(vdso, 0, sizeof(vdso_t));
    vdso->pd = pd;
    vdso->base_addresss = VMM_REGION_U_VDSO;
    vdso->size = PAGE_SIZE_4KIB;
    vdso->entry = 0x0;
    
    create_entry(vdso, VDSO_ENTRY_ID_NONE);
    create_entry(vdso, VDSO_ENTRY_ID_GETTIMEOFDAY);
    create_entry(vdso, VDSO_ENTRY_ID_CLOCK_GETTIME);
    create_entry(vdso, VDSO_ENTRY_SIGNAL_TRAMP);
    return vdso;
}

void vdso_set_data(vdso_t* vdso, uint8_t id, void* info, uint64_t size) {
    vdso_entry_t* entry = vdso->entry;
    while (entry) {
        if (entry->id == id) {
            entry->info = info;
            entry->size = size;
            return;
        }
        entry = entry->next;
    }
}

void vdso_get_data(vdso_t* vdso, uint8_t id, void** info, uint64_t* size) {
    if (info == 0) {
        return;
    }
    vdso_entry_t* entry = vdso->entry;
    while (entry) {
        if (entry->id == id) {
            *info = entry->info;
            if (size != 0) {
                *size = entry->size;
            }
            return;
        }
        entry = entry->next;
    }
    *info = 0;
    *size = 0;
}

void * vdso_allocate_region(vdso_t* vdso, uint64_t size) {
    if (size % PAGE_SIZE_4KIB) {
        size = (size + PAGE_SIZE_4KIB) & ~0xfff;
    }
    if (size > vdso->size) {
        size = vdso->size & ~0xfff;
    }
    
    void* region = allocate_vmm(vdso->pd, size, VMM_REGION_U_VDSO, VMM_WRITE_BIT | VMM_USER_BIT | VMM_CACHE_DISABLE_BIT);
    if (region == 0) {
        panic("Failed to allocate memory for VDSO region\n");
        return 0;
    }
    
    return region;
}

void vdso_free_region(vdso_t* vdso, void* region) {
    free_vmm(vdso->pd, region);
}