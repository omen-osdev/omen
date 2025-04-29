#include <omen/managers/mem/vdso.h>
#include <omen/apps/panic/panic.h>
#include <omen/libraries/std/string.h>

vdso_t * vdso = 0;

void create_entry(vdso_t * vdso, uint8_t id) {
    vdso_entry_t * entry = (vdso_entry_t *)allocate_vmm(vdso->pd, sizeof(vdso_entry_t), VMM_REGION_U_VDSO, VMM_WRITE_BIT | VMM_USER_BIT | VMM_CACHE_DISABLE_BIT);
    if (entry == 0) {
        panic("Failed to allocate memory for VDSO entry\n");
        return;
    }

    memset(entry, 0, sizeof(vdso_entry_t));
    entry->id = id;
    entry->next = vdso->entry;
    vdso->entry = entry;
    entry->info = 0;
    entry->size = 0;
}

vdso_t* vdso_init() {
    struct page_directory * pd = get_pml4();
    extern uint64_t VDSO_START;
    extern uint64_t VDSO_END;
    vdso = (vdso_t*)allocate_vmm(pd, PAGE_SIZE_4KIB, VMM_REGION_U_VDSO, VMM_WRITE_BIT | VMM_USER_BIT | VMM_CACHE_DISABLE_BIT);
    if (vdso == 0) {
        panic("Failed to allocate memory for VDSO\n");
        return 0;
    }

    memset(vdso, 0, sizeof(vdso_t));
    vdso->pd = pd;
    vdso->base_addresss = VDSO_START;
    vdso->size = VDSO_END - VDSO_START;
    vdso->size = (vdso->size + PAGE_SIZE_4KIB) & ~0xfff;
    vdso->entry = 0x0;
    
    create_entry(vdso, VDSO_ENTRY_ID_NONE);
    create_entry(vdso, VDSO_ENTRY_ID_GETTIMEOFDAY);
    create_entry(vdso, VDSO_ENTRY_ID_CLOCK_GETTIME);
    create_entry(vdso, VDSO_ENTRY_SIGNAL_TRAMP);
    create_entry(vdso, VDSO_ENTRY_SIGNAL_SIGNO);
    create_entry(vdso, VDSO_ENTRY_SIGNAL_SIGACTION);
    create_entry(vdso, VDSO_ENTRY_SIGNAL_SIGCTXT);
    return vdso;
}

int vdso_set_data(vdso_t* vdso, uint8_t id, void* info, int64_t size) {
    vdso_entry_t* entry = vdso->entry;
    while (entry) {
        if (entry->id == id) {
            entry->info = info;
            entry->size = size;
            return 0;
        }
        entry = entry->next;
    }

    return -1;
}

int vdso_get_data(vdso_t* vdso, uint8_t id, void** info, int64_t* size) {
    if (info == 0) {
        return -1;
    }
    vdso_entry_t* entry = vdso->entry;
    while (entry) {
        if (entry->id == id) {
            *info = entry->info;
            if (size != 0) {
                *size = entry->size;
            }
            return 0;
        }
        entry = entry->next;
    }
    *info = 0;
    *size = 0;

    return -1;
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

vdso_t* get_vdso() {
    return vdso;
}