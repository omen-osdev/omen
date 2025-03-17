#include <omen/libraries/std/stdint.h>
#include <omen/apps/debug/debug.h>
#include <omen/managers/mem/pmm.h>
#include <omen/managers/mem/vmm.h>
#include <omen/apps/panic/panic.h>
#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/libraries/std/string.h>
#include <generic/config.h>

#define PAGE_SIZE_1GIB      0x40000000
#define PAGE_SIZE_2MIB      0x200000
#define PAGE_SIZE_4KIB      0x1000
#define PAGE_SIZE_DIR       0x1

#define CACHE_BIT_SET(x)((x & PAGE_CACHE_DISABLE_BIT) >> 3)
#define NX_BIT_SET(x)((x & PAGE_NX_BIT) >> 2)
#define USER_BIT_SET(x)((x & PAGE_USER_BIT) >> 1)
#define WRITE_BIT_SET(x)(x & PAGE_WRITE_BIT)
#define COW_BIT_SET(x)(x & PAGE_COW_BIT)

#define PHYSICAL_MEMORY_OFFSET 0xffffA00000000000
#define PHYSICAL_MEMORY_SIZE   0x0000004000000000

#define TO_IDENTITY_MAP(addr) ((addr) + PHYSICAL_MEMORY_OFFSET)
#define FROM_IDENTITY_MAP(addr) ((addr) - PHYSICAL_MEMORY_OFFSET)

#define IS_PRESENT(entry) (entry->directory.P)
#define IS_WRITEABLE(entry) (entry->directory.RW)
#define IS_USER_ACCESS(entry) (entry->directory.US)
#define IS_EXECUTABLE(entry) (!(entry->directory.XD))


#define TO_HUGE_ENTRY(entry) ((struct huge_entry*)entry)
#define TO_BIG_ENTRY(entry) ((struct big_entry*)entry)
#define TO_REGULAR_ENTRY(entry) ((struct directory_entry*)entry)
#define TO_DIRECTORY_ENTRY(entry) ((struct directory_entry*)entry)

#define GET_PPDP(entry) (entry->directory.PDPP)

void address_to_map(uint64_t address, struct page_map_index* map) {
    address >>= 12;
    map->PT_index = address & 0x1ff;
    address >>= 9;
    map->PD_index = address & 0x1ff;
    address >>= 9;
    map->PDP_index = address & 0x1ff;
    address >>= 9;    
    map->PML4_index = address & 0x1ff;
}

void map_to_address(struct page_map_index* map, uint64_t* address) {
    *address = map->PML4_index;
    *address <<= 9;
    *address |= map->PDP_index;
    *address <<= 9;
    *address |= map->PD_index;
    *address <<= 9;
    *address |= map->PT_index;
    *address <<= 12;
}

void switch_cr3(struct page_directory* cr3)
{
    __asm__("mov %0, %%cr3" : : "r"(cr3));
}

void flush_tlb_entry(void* address)
{
    __asm__("invlpg (%0)" : : "r"(address) : "memory");
}

struct page_directory* get_current_cr3()
{
    struct page_directory* cr3;
    __asm__("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

void init_entry(vm_entry * entry, uint64_t size, uint64_t page_ppn)
{   
    entry->directory.P = 1;          //0
    entry->directory.RW = 1;         //1
    entry->directory.US = 1;         //2
    entry->directory.PWT = 0;        //3
    entry->directory.PCD = 0;        //4
    entry->directory.A = 0;          //5
    entry->directory.D = 0;          //6
    entry->directory.PS = 0;         //7
    entry->directory.IGNORED2 = 0;   //8-10
    entry->directory.R = 0;          //11
    entry->directory.PDPP = 0;         //12-39 Provisionally set to 0
    entry->directory.RESERVED = 0;   //40-51
    entry->directory.IGNORED3 = 0;   //52-62
    entry->directory.XD = 0;           //63

    switch (size)
    {
        case PAGE_SIZE_1GIB:
            entry.huge.PS = 1;
            //entry.huge.PPDP is 10 bits
            entry.huge.PPDP = page_ppn >> 30;
            break;
        case PAGE_SIZE_2MIB:
            entry.big.PS = 1;
            //entry.big.PDPP is 19 bits
            entry.big.PDPP = page_ppn >> 21;
            break;
        case PAGE_SIZE_4KIB:
            //entry.regular.PDPP is 28 bits
            entry.regular.PDPP = page_ppn >> 12;
            break;
    }
}

#define GET_ENTRY(root, index) ((struct vm_entry*)&(root->entries[index]))

void map_address(struct page_directory* root, void * virtual_address, void * physical_address, uint64_t size)
{
    struct page_map_index map;
    address_to_map((uint64_t)virtual_address, &map);

    struct page_directory *pdptable, *pdtable, *pttable;
    vm_entry *pml4entry, *pdptentry, *pdentry, *ptentry;

    pml4entry = GET_ENTRY(root, map.PML4_index);
    if (!IS_PRESENT(pml4entry)) {
        init_entry(pml4entry, PAGE_SIZE_DIR, (uint64_t)pmm_alloc_page());
    }

    pdptable = (struct page_table*)((uint64_t)GET_PPDP(pml4entry) << 12);
    pdptentry = GET_ENTRY(pdptable, map.PDP_index);

    if (!IS_PRESENT(pdptentry)) {
        if (size == PAGE_SIZE_1GIB) {
            init_entry(pdptentry, PAGE_SIZE_1GIB, (uint64_t)physical_address);
            flush_tlb_entry(virtual_address);
            return;
        } else {
            init_entry(pdptentry, PAGE_SIZE_DIR, (uint64_t)pmm_alloc_page());
        }
    } else if (TO_HUGE_ENTRY(pdptentry)->PS) {
        if (size == PAGE_SIZE_1GIB) {
            init_entry(pdptentry, PAGE_SIZE_1GIB, (uint64_t)physical_address);
            flush_tlb_entry(virtual_address);
            return;
        } else {
            kprintf("%llx 1g Mapping overlap detected, trying to map: %llx, already mapped: %llx\n", virtual_address, physical_address, TO_HUGE_ENTRY(pdptentry)->PPDP << 30);
            panic("1GiB Mapping overlap detected\n");
        }
    }

    pdtable = (struct page_table*)((uint64_t)GET_PPDP(pdptentry) << 12);
    pdentry = GET_ENTRY(pdtable, map.PD_index);

    if (!IS_PRESENT(pdentry)) {
        if (size == PAGE_SIZE_2MIB) {
            init_entry(pdentry, PAGE_SIZE_2MIB, (uint64_t)physical_address);
            flush_tlb_entry(virtual_address);
            return;
        } else {
            init_entry(pdentry, PAGE_SIZE_DIR, (uint64_t)pmm_alloc_page());
        }
    } else if (TO_BIG_ENTRY(pdentry)->PS) {
        if (size == PAGE_SIZE_2MIB) {
            init_entry(pdentry, PAGE_SIZE_2MIB, (uint64_t)physical_address);
            flush_tlb_entry(virtual_address);
            return;
        } else {
            kprintf("%llx 2m Mapping overlap detected, trying to map: %llx, already mapped: %llx\n", virtual_address, physical_address, TO_BIG_ENTRY(pdentry)->PDPP << 21);
            panic("2MiB Mapping overlap detected\n");
        }
    }

    pttable = (struct page_table*)((uint64_t)GET_PPDP(pdentry) << 12);
    ptentry = GET_ENTRY(pttable, map.PT_index);

    if (!IS_PRESENT(ptentry)) {
        init_entry(ptentry, PAGE_SIZE_4KIB, (uint64_t)physical_address);
        flush_tlb_entry(virtual_address);
        return;
    } else {
        kprintf("%llx 4k Mapping overlap detected, trying to map: %llx, already mapped: %llx\n", virtual_address, physical_address, TO_REGULAR_ENTRY(ptentry)->PDPP << 12);
        panic("4KiB Mapping overlap detected\n");
    }

    flush_tlb_entry(virtual_address);
}

void * allocate_vmm_page(struct page_directory * pml4, uint8_t flags)
{
    void * buffer = TO_IDENTITY_MAP(pmm_alloc_page());
    if (FROM_IDENTITY_MAP(buffer) == NULL)
    {
        return NULL;
    }
    //mprotect(pml4, buffer, PAGE_SIZE_4KIB, flags);
    return buffer;
}

void free_vmm_page(struct page_directory * pml4, void * address)
{
    if (FROM_IDENTITY_MAP(address) == NULL)
    {
        return;
    }
    pmm_free(FROM_IDENTITY_MAP(address));
    unmap_memory(pml4, address);
}

void * allocate_vmm(struct page_directory * pml4, uint64_t size, uint8_t flags)
{
    void * buffer = TO_IDENTITY_MAP(pmm_alloc(size));
    if (FROM_IDENTITY_MAP(buffer) == NULL)
    {
        return NULL;
    }
    //mprotect(pml4, buffer, size, flags);
    return buffer;
}

void free_vmm(struct page_directory * pml4, void * address)
{
    if (FROM_IDENTITY_MAP(address) == NULL)
    {
        return;
    }
    pmm_free(FROM_IDENTITY_MAP(address));
    unmap_memory(pml4, address);
}

void map_range(struct page_directory* root, void * virtual_start, void * physical_start, uint64_t page_size, uint64_t size)
{
    //number of pages to map
    uint64_t pages = size / page_size;
    if (size % page_size)
    {
        pages++;
    }
    kprintf("Need to map %d pages\n", pages);
    for (uint64_t i = 0; i < pages; i++)
    {
        map_address(root, (void*)((uint64_t)virtual_start + (i * page_size)), (void*)((uint64_t)physical_start + (i * page_size)), page_size);
    }

    kprintf("Mapped range from 0x%llx to 0x%llx\n", virtual_start, (uint64_t)virtual_start + size);
}

void duplicate_page_directory(struct page_directory* root, struct page_directory* new, uint8_t level)
{
    for (int i = 0; i < 512; i++)
    {
        struct page_directory_entry* entry = &(root->entries[i]);
        if (entry->present)
        {
            (&(new->entries[i]))->present = entry->present;
            (&(new->entries[i]))->writeable = entry->writeable;
            (&(new->entries[i]))->user_access = entry->user_access;
            (&(new->entries[i]))->write_through = entry->write_through;
            (&(new->entries[i]))->cache_disabled = entry->cache_disabled;
            (&(new->entries[i]))->accessed = entry->accessed;
            (&(new->entries[i]))->ignored_3 = entry->ignored_3;
            (&(new->entries[i]))->size = entry->size;
            (&(new->entries[i]))->global = entry->global;
            (&(new->entries[i]))->cow = entry->cow;
            (&(new->entries[i]))->reserved_1 = entry->reserved_1;
            (&(new->entries[i]))->ignored_1 = entry->ignored_1;
            (&(new->entries[i]))->execution_disabled = entry->execution_disabled;

            if (level == 0 || entry->size)
            {
                (&(new->entries[i]))->page_ppn = entry->page_ppn;
            } else {
                (&(new->entries[i]))->page_ppn = ((uint64_t)pmm_alloc_page()) >> 12;
                duplicate_page_directory(
                    (struct page_directory*)((uint64_t)entry->page_ppn << 12),
                    (struct page_directory*)((uint64_t)(&(new->entries[i]))->page_ppn << 12),
                    level - 1
                );
            }
        }
    }
}

void duplicate_cr3(struct page_directory* root, struct page_directory* new)
{
    duplicate_page_directory(root, new, 3);
}

void unmap_address(struct page_directory* root, void* virtual_address)
{
    struct page_map_index map;
    address_to_map((uint64_t)virtual_address, &map);

    struct page_directory* pml4 = root;
    struct page_directory_entry* pml4entry = (struct page_directory_entry*)&(pml4->entries[map.PML4_index]);
    if (!pml4entry->present)
    {
        return;
    }

    struct page_table* pdptable = (struct page_table*)((uint64_t)pml4entry->page_ppn << 12);
    struct page_table_entry* pdptentry = (struct page_table_entry*)&(pdptable->entries[map.PDP_index]);

    if (!pdptentry->present)
    {
        return;
    }

    struct page_table* pdtable = (struct page_table*)((uint64_t)pdptentry->page_ppn << 12);
    struct page_table_entry* pdentry = (struct page_table_entry*)&(pdtable->entries[map.PD_index]);

    if (!pdentry->present)
    {
        return;
    }

    struct page_table* pttable = (struct page_table*)((uint64_t)pdentry->page_ppn << 12);
    struct page_table_entry* ptentry = (struct page_table_entry*)&(pttable->entries[map.PT_index]);

    if (!ptentry->present)
    {
        return;
    }

    ptentry->present = 0;
    flush_tlb_entry(virtual_address);
}

void* get_physical_address(struct page_directory* cr3, void* virtual_address)
{
    struct page_map_index map;
    address_to_map((uint64_t)virtual_address, &map);

    kprintf("[DEBUG] Virtual address: 0x%llx\n", virtual_address);
    kprintf("[DEBUG] PML4 index: %d\n", map.PML4_index);
    kprintf("[DEBUG] PDP index: %d\n", map.PDP_index);
    kprintf("[DEBUG] PD index: %d\n", map.PD_index);
    kprintf("[DEBUG] PT index: %d\n", map.PT_index);
    kprintf("[DEBUG] Offset: %llx\n", (uint64_t)virtual_address & 0xfff);

    struct page_directory* pml4 = cr3;
    struct page_directory_entry* pml4entry = (struct page_directory_entry*)&(pml4->entries[map.PML4_index]);
    if (!pml4entry->present)
    {
        panic("[DEBUG][PML4] pml4->entries[PML4_index] not present\n");
    }

    struct page_table* pdptable = (struct page_table*)((uint64_t)pml4entry->page_ppn << 12);
    struct page_table_entry* pdptentry = (struct page_table_entry*)&(pdptable->entries[map.PDP_index]);

    if (!pdptentry->present)
    {
        panic("[DEBUG][PDP] pdptable->entries[PDP_index] not present\n");
        return NULL;
    } else if (pdptentry->size)
    {
        kprintf("[DEBUG][PDP] pdptable->entries[PDP_index] is a 1GiB page\n");
        return (void*)((uint64_t)pdptentry->page_ppn << 18 | ((uint64_t)virtual_address & 0x3fffff));
    }

    struct page_table* pdtable = (struct page_table*)((uint64_t)pdptentry->page_ppn << 12);
    struct page_table_entry* pdentry = (struct page_table_entry*)&(pdtable->entries[map.PD_index]);

    if (!pdentry->present)
    {
        panic("[DEBUG][PD] pdtable->entries[PD_index] not present\n");
        return NULL;
    } else if (pdentry->size)
    {
        kprintf("[DEBUG][PD] pdtable->entries[PD_index] is a 2MiB page\n");
        return (void*)((uint64_t)pdentry->page_ppn << 12 | ((uint64_t)virtual_address & 0x1fffff));
    }

    struct page_table* pttable = (struct page_table*)((uint64_t)pdentry->page_ppn << 12);
    struct page_table_entry* ptentry = (struct page_table_entry*)&(pttable->entries[map.PT_index]);

    if (!ptentry->present)
    {
        panic("[DEBUG][PT] pttable->entries[PT_index] not present\n");
        return NULL;
    }

    return (void*)((uint64_t)ptentry->page_ppn << 12 | ((uint64_t)virtual_address & 0xfff));
}

void init_vmm()
{

    struct page_directory* global_cr3 = pmm_alloc_page();
    memcpy(global_cr3, get_current_cr3(), sizeof(struct page_directory));
    map_range(global_cr3, (void*)PHYSICAL_MEMORY_OFFSET, (void*)0, PAGE_SIZE_1GIB, PHYSICAL_MEMORY_SIZE);
    switch_cr3(global_cr3);
    kprintf("Page table switched\n");
    
    void * addr = TO_IDENTITY_MAP(pmm_alloc_page());
    kprintf("Allocated page at 0x%llx\n", addr);

    debug_address(global_cr3, addr);

    map_address(global_cr3, (void*)0xffff900000000000, (void*)FROM_IDENTITY_MAP(addr), PAGE_SIZE_4KIB);
    memset((void*)0xffff900000000000, 0x42, PAGE_SIZE_4KIB);
    
    void *physical_address = get_physical_address(global_cr3, (void*)0xffff900000000000);
    kprintf("Physical address: 0x%llx\n", physical_address);
    kprintf("Virtual address value: 0x%llx\n", *((uint64_t*)0xffff900000000000));

    physical_address = get_physical_address(global_cr3, addr);
    kprintf("Physical address: 0x%llx\n", physical_address);
    kprintf("Virtual address value: 0x%llx\n", *((uint64_t*)addr));

}

//Adapters for the interface
struct page_directory* get_pml4() {
    return get_current_cr3();
}

uint8_t remap_allocate_cow(struct page_directory * pml4, void * address_raw) {
    panic("Not implemented\n");
    return 0;
}

struct page_directory * duplicate_pd(struct page_directory * pml4, uint8_t share_kernel, uint8_t use_cow) {
    
    if (use_cow)
    {
        panic("COW not implemented\n");
        return NULL;
    }

    struct page_directory* new_pml4 = pmm_alloc_page();
    if (share_kernel)
    {
        for (int i = 256; i < 512; i++)
        {
            if (pml4->entries[i].present)
            {
                (&(new_pml4->entries[i]))->present = pml4->entries[i].present;
                (&(new_pml4->entries[i]))->writeable = pml4->entries[i].writeable;
                (&(new_pml4->entries[i]))->user_access = pml4->entries[i].user_access;
                (&(new_pml4->entries[i]))->write_through = pml4->entries[i].write_through;
                (&(new_pml4->entries[i]))->cache_disabled = pml4->entries[i].cache_disabled;
                (&(new_pml4->entries[i]))->accessed = pml4->entries[i].accessed;
                (&(new_pml4->entries[i]))->ignored_3 = pml4->entries[i].ignored_3;
                (&(new_pml4->entries[i]))->size = pml4->entries[i].size;
                (&(new_pml4->entries[i]))->global = pml4->entries[i].global;
                (&(new_pml4->entries[i]))->cow = pml4->entries[i].cow;
                (&(new_pml4->entries[i]))->page_ppn = pml4->entries[i].page_ppn;
                (&(new_pml4->entries[i]))->reserved_1 = pml4->entries[i].reserved_1;
                (&(new_pml4->entries[i]))->ignored_1 = pml4->entries[i].ignored_1;
                (&(new_pml4->entries[i]))->execution_disabled = pml4->entries[i].execution_disabled;

                duplicate_page_directory(
                    (struct page_directory*)((uint64_t)pml4->entries[i].page_ppn << 12),
                    (struct page_directory*)((uint64_t)(&(new_pml4->entries[i]))->page_ppn << 12),
                    2
                );
            }
        }
    } else {
        duplicate_cr3(pml4, new_pml4);
    }

    return new_pml4;
}

void map_memory(struct page_directory * pml4, void * address, void * physical, uint64_t page_size, uint8_t flags) {
    map_address(pml4, address, physical, page_size);
}


struct page_directory* duplicate_current_pml4() {
    struct page_directory* pml4 = get_pml4();
    struct page_directory* new_pml4 = pmm_alloc_page();
    duplicate_cr3(pml4, new_pml4);
    return new_pml4;
}

//Only internal use
void set_pml4(struct page_directory* pml4) {
    switch_cr3(pml4);
}
void * virtual_to_physical(struct page_directory * pml4, void * address) {
    return get_physical_address(pml4, address);
}
void unmap_memory(struct page_directory * pml4, void * address) {
    unmap_address(pml4, address);
}

//Unused
void invalidate_current_pml4() {
    struct page_directory* pml4 = get_current_cr3();
    __asm__("mov %0, %%cr3" : : "r"(pml4));
}
//Used one time
void init_paging() {
    init_vmm();
}

void set_permissions(struct page_table_entry* entry, uint8_t flags)
{
    entry->writeable = WRITE_BIT_SET(flags);
    entry->user_access = USER_BIT_SET(flags);
    entry->cache_disabled = CACHE_BIT_SET(flags);
    entry->execution_disabled = NX_BIT_SET(flags);
    entry->cow = COW_BIT_SET(flags);
}

uint64_t mprotect_page(struct page_directory * root, void* address, uint8_t flags)
{
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    kprintf("Mprotecting vaddr: %llx (phys: %llx) with flags: %x\n", address, get_physical_address(root, address), flags);

    struct page_directory* pml4 = root;
    struct page_directory_entry* pml4entry = (struct page_directory_entry*)&(pml4->entries[map.PML4_index]);
    if (!pml4entry->present)
    {
        return 0;
    } else if (pml4entry->size)
    {
        return 0;
    }
    struct page_table* pdptable = (struct page_table*)((uint64_t)pml4entry->page_ppn << 12);
    struct page_table_entry* pdptentry = (struct page_table_entry*)&(pdptable->entries[map.PDP_index]);

    if (!pdptentry->present)
    {
        return 0;
    } else if (pdptentry->size)
    {
        set_permissions(pdptentry, flags);
        if (USER_BIT_SET(flags))
        {
            pml4entry->user_access = 1;
            pdptentry->user_access = 1;
        }
        return PAGE_SIZE_1GIB;
    }

    struct page_table* pdtable = (struct page_table*)((uint64_t)pdptentry->page_ppn << 12);
    struct page_table_entry* pdentry = (struct page_table_entry*)&(pdtable->entries[map.PD_index]);

    if (!pdentry->present)
    {
        return 0;
    } else if (pdentry->size)
    {
        set_permissions(pdentry, flags);
        if (USER_BIT_SET(flags))
        {
            pml4entry->user_access = 1;
            pdptentry->user_access = 1;
            pdentry->user_access = 1;
        }
        return PAGE_SIZE_2MIB;
    }

    struct page_table* pttable = (struct page_table*)((uint64_t)pdentry->page_ppn << 12);
    struct page_table_entry* ptentry = (struct page_table_entry*)&(pttable->entries[map.PT_index]);

    if (!ptentry->present)
    {
        return 0;
    }

    set_permissions(ptentry, flags);
    if (USER_BIT_SET(flags))
    {
        pml4entry->user_access = 1;
        pdptentry->user_access = 1;
        pdentry->user_access = 1;
        ptentry->user_access = 1;
    }

    return PAGE_SIZE_4KIB;
}

void mprotect(struct page_directory * root, void* address, uint64_t size, uint8_t flags)
{
    uint64_t next_address = (uint64_t)address >> 12;
    next_address <<= 12;
    while (next_address < (uint64_t)address + size)
    {
        uint64_t page_size = mprotect_page(root, (void*)next_address, flags);
        if (!page_size)
        {
            kprintf("Failed to protect page at 0x%llx\n", next_address);
            panic("Failed to protect page\n");
        }
        next_address += page_size;
    }
}


uint8_t is_present(struct page_directory* pml4, void * address) {
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory* pml4table = pml4;
    struct page_directory_entry* pml4entry = (struct page_directory_entry*)&(pml4table->entries[map.PML4_index]);
    if (!pml4entry->present)
    {
        return 0;
    }

    struct page_table* pdptable = (struct page_table*)((uint64_t)pml4entry->page_ppn << 12);
    struct page_table_entry* pdptentry = (struct page_table_entry*)&(pdptable->entries[map.PDP_index]);

    if (!pdptentry->present)
    {
        return 0;
    } else if (pdptentry->size)
    {
        return 1;
    }

    struct page_table* pdtable = (struct page_table*)((uint64_t)pdptentry->page_ppn << 12);
    struct page_table_entry* pdentry = (struct page_table_entry*)&(pdtable->entries[map.PD_index]);

    if (!pdentry->present)
    {
        return 0;
    } else if (pdentry->size)
    {
        return 1;
    }

    struct page_table* pttable = (struct page_table*)((uint64_t)pdentry->page_ppn << 12);
    struct page_table_entry* ptentry = (struct page_table_entry*)&(pttable->entries[map.PT_index]);

    if (!ptentry->present)
    {
        return 0;
    }

    return 1;
}
uint8_t is_writeable(struct page_directory* pml4, void * address)
{
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory* pml4table = pml4;
    struct page_directory_entry* pml4entry = (struct page_directory_entry*)&(pml4table->entries[map.PML4_index]);
    if (!pml4entry->present)
    {
        return 0;
    }

    struct page_table* pdptable = (struct page_table*)((uint64_t)pml4entry->page_ppn << 12);
    struct page_table_entry* pdptentry = (struct page_table_entry*)&(pdptable->entries[map.PDP_index]);

    if (!pdptentry->present)
    {
        return 0;
    } else if (pdptentry->size)
    {
        return pdptentry->writeable;
    }

    struct page_table* pdtable = (struct page_table*)((uint64_t)pdptentry->page_ppn << 12);
    struct page_table_entry* pdentry = (struct page_table_entry*)&(pdtable->entries[map.PD_index]);

    if (!pdentry->present)
    {
        return 0;
    } else if (pdentry->size)
    {
        return pdentry->writeable;
    }

    struct page_table* pttable = (struct page_table*)((uint64_t)pdentry->page_ppn << 12);
    struct page_table_entry* ptentry = (struct page_table_entry*)&(pttable->entries[map.PT_index]);

    if (!ptentry->present)
    {
        return 0;
    }

    return ptentry->writeable;
}
uint8_t is_user_access(struct page_directory* pml4, void * address) {
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory* pml4table = pml4;
    struct page_directory_entry* pml4entry = (struct page_directory_entry*)&(pml4table->entries[map.PML4_index]);
    if (!pml4entry->present)
    {
        return 0;
    }

    struct page_table* pdptable = (struct page_table*)((uint64_t)pml4entry->page_ppn << 12);
    struct page_table_entry* pdptentry = (struct page_table_entry*)&(pdptable->entries[map.PDP_index]);

    if (!pdptentry->present)
    {
        return 0;
    } else if (pdptentry->size)
    {
        return pdptentry->user_access && pml4entry->user_access;
    }

    struct page_table* pdtable = (struct page_table*)((uint64_t)pdptentry->page_ppn << 12);
    struct page_table_entry* pdentry = (struct page_table_entry*)&(pdtable->entries[map.PD_index]);

    if (!pdentry->present)
    {
        return 0;
    } else if (pdentry->size)
    {
        return pdentry->user_access && pdptentry->user_access && pml4entry->user_access;
    }

    struct page_table* pttable = (struct page_table*)((uint64_t)pdentry->page_ppn << 12);
    struct page_table_entry* ptentry = (struct page_table_entry*)&(pttable->entries[map.PT_index]);

    if (!ptentry->present)
    {
        return 0;
    }

    return ptentry->user_access && pdentry->user_access && pdptentry->user_access && pml4entry->user_access;
}
uint8_t is_executable(struct page_directory* pml4, void * address)
{
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory* pml4table = pml4;
    struct page_directory_entry* pml4entry = (struct page_directory_entry*)&(pml4table->entries[map.PML4_index]);
    if (!pml4entry->present)
    {
        return 0;
    }

    struct page_table* pdptable = (struct page_table*)((uint64_t)pml4entry->page_ppn << 12);
    struct page_table_entry* pdptentry = (struct page_table_entry*)&(pdptable->entries[map.PDP_index]);

    if (!pdptentry->present)
    {
        return 0;
    } else if (pdptentry->size)
    {
        return !pdptentry->execution_disabled;
    }

    struct page_table* pdtable = (struct page_table*)((uint64_t)pdptentry->page_ppn << 12);
    struct page_table_entry* pdentry = (struct page_table_entry*)&(pdtable->entries[map.PD_index]);

    if (!pdentry->present)
    {
        return 0;
    } else if (pdentry->size)
    {
        return !pdentry->execution_disabled;
    }

    struct page_table* pttable = (struct page_table*)((uint64_t)pdentry->page_ppn << 12);
    struct page_table_entry* ptentry = (struct page_table_entry*)&(pttable->entries[map.PT_index]);

    if (!ptentry->present)
    {
        return 0;
    }

    return !ptentry->execution_disabled;
}
uint8_t get_page_perms(struct page_directory *pml4, void* address) {
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory* pml4table = pml4;
    struct page_directory_entry* pml4entry = (struct page_directory_entry*)&(pml4table->entries[map.PML4_index]);
    if (!pml4entry->present)
    {
        return 0;
    }

    struct page_table* pdptable = (struct page_table*)((uint64_t)pml4entry->page_ppn << 12);
    struct page_table_entry* pdptentry = (struct page_table_entry*)&(pdptable->entries[map.PDP_index]);

    if (!pdptentry->present)
    {
        return 0;
    } else if (pdptentry->size)
    {
        return pdptentry->writeable | (pdptentry->user_access << 1) | (pdptentry->cache_disabled << 2) | (pdptentry->execution_disabled << 3);
    }

    struct page_table* pdtable = (struct page_table*)((uint64_t)pdptentry->page_ppn << 12);
    struct page_table_entry* pdentry = (struct page_table_entry*)&(pdtable->entries[map.PD_index]);

    if (!pdentry->present)
    {
        return 0;
    } else if (pdentry->size)
    {
        return pdentry->writeable | (pdentry->user_access << 1) | (pdentry->cache_disabled << 2) | (pdentry->execution_disabled << 3);
    }

    struct page_table* pttable = (struct page_table*)((uint64_t)pdentry->page_ppn << 12);
    struct page_table_entry* ptentry = (struct page_table_entry*)&(pttable->entries[map.PT_index]);

    if (!ptentry->present)
    {
        return 0;
    }

    return ptentry->writeable | (ptentry->user_access << 1) | (ptentry->cache_disabled << 2) | (ptentry->execution_disabled << 3);
}