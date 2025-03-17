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

#define PHYSICAL_MEMORY_OFFSET 0xffffA00000000000
#define PHYSICAL_MEMORY_SIZE   0x0000004000000000

#define TO_IDENTITY_MAP(addr) ((addr) + PHYSICAL_MEMORY_OFFSET)
#define FROM_IDENTITY_MAP(addr) ((addr) - PHYSICAL_MEMORY_OFFSET)

#define CACHE_BIT_SET(x)((x & PAGE_CACHE_DISABLE_BIT) >> 3)
#define NX_BIT_SET(x)((x & PAGE_NX_BIT) >> 2)
#define USER_BIT_SET(x)((x & PAGE_USER_BIT) >> 1)
#define WRITE_BIT_SET(x)(x & PAGE_WRITE_BIT)

#define IS_PRESENT(entry) ((entry)->directory.P)
#define IS_WRITEABLE(entry) ((entry)->directory.RW)
#define IS_USER_ACCESS(entry) ((entry)->directory.US)
#define IS_EXECUTABLE(entry) (!((entry)->directory.XD))

#define GET_PDPP_HUGE(entry) ((uint64_t)(((uint64_t)(entry)->huge.PDPP) << 30))
#define GET_PDPP_BIG(entry) ((uint64_t)(((uint64_t)(entry)->big.PDPP) << 21))
#define GET_PDPP_REGULAR(entry) ((uint64_t)(((uint64_t)(entry)->regular.PDPP) << 12))
#define GET_PDPP_DIR(entry) ((uint64_t)(((uint64_t)(entry)->directory.PDPP) << 12))
#define GET_ENTRY(root, index) (vm_entry*)(&((root)->entries[(index)]))

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

uint64_t get_pdpp(vm_entry * entry, uint64_t size)
{
    switch (size)
    {
        case PAGE_SIZE_1GIB:
            return GET_PDPP_HUGE(entry);
        case PAGE_SIZE_2MIB:
            return GET_PDPP_BIG(entry);
        case PAGE_SIZE_4KIB:
            return GET_PDPP_REGULAR(entry);
        default:
            return GET_PDPP_DIR(entry);
    }
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
    entry->directory.IGNORED1 = 0;          //6
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
            entry->huge.PS = 1;
            entry->huge.PDPP = page_ppn >> 30;
            break;
        case PAGE_SIZE_2MIB:
            entry->big.PS = 1;
            entry->big.PDPP = page_ppn >> 21;
            break;
        case PAGE_SIZE_4KIB:
            entry->regular.PDPP = page_ppn >> 12;
            break;
        default:
            entry->directory.PDPP = page_ppn >> 12;
            break;
    }
}

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

    pdptable = (struct page_directory*)get_pdpp(pml4entry, PAGE_SIZE_DIR);
    pdptentry = GET_ENTRY(pdptable, map.PDP_index);

    if (!IS_PRESENT(pdptentry)) {
        if (size == PAGE_SIZE_1GIB) {
            init_entry(pdptentry, PAGE_SIZE_1GIB, (uint64_t)physical_address);
            flush_tlb_entry(virtual_address);
            return;
        } else {
            init_entry(pdptentry, PAGE_SIZE_DIR, (uint64_t)pmm_alloc_page());
        }
    } else if (pdptentry->huge.PS) {
        if (size == PAGE_SIZE_1GIB) {
            init_entry(pdptentry, PAGE_SIZE_1GIB, (uint64_t)physical_address);
            flush_tlb_entry(virtual_address);
            kprintf("1g Mapping overlap detected, trying to map: %llx, already mapped: %llx\n", virtual_address, get_pdpp(pdptentry, PAGE_SIZE_1GIB));
            return;
        } else {
            kprintf("%llx 1g Mapping overlap detected, trying to map: %llx, already mapped: %llx\n", virtual_address, physical_address, get_pdpp(pdptentry, PAGE_SIZE_1GIB));
            panic("1GiB Mapping overlap detected\n");
        }
    }

    pdtable = (struct page_directory*)get_pdpp(pdptentry, PAGE_SIZE_DIR);
    pdentry = GET_ENTRY(pdtable, map.PD_index);

    if (!IS_PRESENT(pdentry)) {
        if (size == PAGE_SIZE_2MIB) {
            init_entry(pdentry, PAGE_SIZE_2MIB, (uint64_t)physical_address);
            flush_tlb_entry(virtual_address);
            return;
        } else {
            init_entry(pdentry, PAGE_SIZE_DIR, (uint64_t)pmm_alloc_page());
        }
    } else if (pdptentry->big.PS) {
        if (size == PAGE_SIZE_2MIB) {
            init_entry(pdentry, PAGE_SIZE_2MIB, (uint64_t)physical_address);
            flush_tlb_entry(virtual_address);
            kprintf("2m Mapping overlap detected, trying to map: %llx, already mapped: %llx\n", virtual_address, get_pdpp(pdentry, PAGE_SIZE_2MIB));
            return;
        } else {
            kprintf("2m Mapping overlap detected, trying to map: %llx, already mapped: %llx\n", virtual_address, physical_address);
            panic("2MiB Mapping overlap detected\n");
        }
    }

    pttable = (struct page_directory*)get_pdpp(pdentry, PAGE_SIZE_DIR);
    ptentry = GET_ENTRY(pttable, map.PT_index);

    if (!IS_PRESENT(ptentry)) {
        init_entry(ptentry, PAGE_SIZE_4KIB, (uint64_t)physical_address);
        flush_tlb_entry(virtual_address);
        return;
    } else {
        kprintf("4k Mapping overlap detected, trying to map: %llx, already mapped: %llx\n", virtual_address, physical_address);
        init_entry(ptentry, PAGE_SIZE_4KIB, (uint64_t)physical_address);
        flush_tlb_entry(virtual_address);
    }
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

void * allocate_current_vmm(uint64_t size, uint8_t flags)
{
    return allocate_vmm(get_current_cr3(), size, flags);
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

void * free_current_vmm(void * address) {
    return free_vmm(get_current_cr3(), address);
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
        vm_entry* entry = GET_ENTRY(root, i);
        vm_entry* new_entry = GET_ENTRY(new, i);
        if (IS_PRESENT(entry))
        {
            new_entry->directory.P = entry->directory.P;
            new_entry->directory.RW = entry->directory.RW;
            new_entry->directory.US = entry->directory.US;
            new_entry->directory.PWT = entry->directory.PWT;
            new_entry->directory.PCD = entry->directory.PCD;
            new_entry->directory.A = entry->directory.A;
            new_entry->directory.IGNORED1 = entry->directory.IGNORED1;
            new_entry->directory.PS = entry->directory.PS;
            new_entry->directory.IGNORED2 = entry->directory.IGNORED2;
            new_entry->directory.R = entry->directory.R;
            new_entry->directory.PDPP = 0; //CHANGED LATED
            new_entry->directory.RESERVED = entry->directory.RESERVED;
            new_entry->directory.IGNORED3 = entry->directory.IGNORED3;
            new_entry->directory.XD = entry->directory.XD;

            if (level == 0 || entry->directory.PS)
            {
                new_entry->directory.PDPP = entry->directory.PDPP;
            } else {
                new_entry->directory.PDPP = ((uint64_t)pmm_alloc_page()) >> 12;
                duplicate_page_directory(
                    (struct page_directory*)get_pdpp(entry, PAGE_SIZE_DIR),
                    (struct page_directory*)get_pdpp(new_entry, PAGE_SIZE_DIR),
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

    struct page_directory *pdptable, *pdtable, *pttable;
    vm_entry *pml4entry, *pdptentry, *pdentry, *ptentry;

    pml4entry = GET_ENTRY(root, map.PML4_index);
    if (!IS_PRESENT(pml4entry))
    {
        return;
    }

    pdptable = (struct page_directory*)get_pdpp(pml4entry, PAGE_SIZE_DIR);
    pdptentry = GET_ENTRY(pdptable, map.PDP_index);

    if (!IS_PRESENT(pdptentry))
    {
        return;
    }
    
    if (pdptentry->huge.PS)
    {
        pdptentry->directory.P = 0;
        return;
    }

    pdtable = (struct page_directory*)get_pdpp(pdptentry, PAGE_SIZE_DIR);
    pdentry = GET_ENTRY(pdtable, map.PD_index);

    if (!IS_PRESENT(pdentry))
    {
        return;
    }
    if (pdptentry->big.PS)
    {
        pdptentry->directory.P = 0;
        return;
    }

    pttable = (struct page_directory*)get_pdpp(pdentry, PAGE_SIZE_DIR);
    ptentry = GET_ENTRY(pttable, map.PT_index);

    if (!IS_PRESENT(ptentry))
    {
        return;
    }

    ptentry->directory.P = 0;
}
    

void* get_physical_address(struct page_directory* root, void* virtual_address)
{
    struct page_map_index map;
    address_to_map((uint64_t)virtual_address, &map);

    kprintf("[DEBUG] Virtual address: 0x%llx\n", virtual_address);
    kprintf("[DEBUG] PML4 index: %d\n", map.PML4_index);
    kprintf("[DEBUG] PDP index: %d\n", map.PDP_index);
    kprintf("[DEBUG] PD index: %d\n", map.PD_index);
    kprintf("[DEBUG] PT index: %d\n", map.PT_index);
    kprintf("[DEBUG] Offset: %llx\n", (uint64_t)virtual_address & 0xfff);

    struct page_directory* pdptable, *pdtable, *pttable;
    vm_entry *pml4entry, *pdptentry, *pdentry, *ptentry;
    pml4entry = GET_ENTRY(root, map.PML4_index);
    if (!IS_PRESENT(pml4entry))
    {
        panic("[DEBUG][PML4] pml4->entries[PML4_index] not present\n");
    }

    pdptable = (struct page_directory*)get_pdpp(pml4entry, PAGE_SIZE_DIR);
    pdptentry = GET_ENTRY(pdptable, map.PDP_index);

    if (!IS_PRESENT(pdptentry))
    {
        panic("[DEBUG][PDP] pdptable->entries[PDP_index] not present\n");
    } else if (pdptentry->huge.PS)
    {
        kprintf("[DEBUG][PDP] pdptable->entries[PDP_index] is a 1GiB page\n");
        return (void*)(get_pdpp(pdptentry, PAGE_SIZE_1GIB) | ((uint64_t)virtual_address & 0x3fffffff));
    }
    
    pdtable = (struct page_directory*)get_pdpp(pdptentry, PAGE_SIZE_DIR);
    pdentry = GET_ENTRY(pdtable, map.PD_index);

    if (!IS_PRESENT(pdentry))
    {
        panic("[DEBUG][PD] pdtable->entries[PD_index] not present\n");
    } else if (pdptentry->big.PS)
    {
        kprintf("[DEBUG][PD] pdtable->entries[PD_index] is a 2MiB page\n");
        return (void*)(get_pdpp(pdentry, PAGE_SIZE_2MIB) | ((uint64_t)virtual_address & 0x1fffff));
    }

    pttable = (struct page_directory*)get_pdpp(pdentry, PAGE_SIZE_DIR);
    ptentry = GET_ENTRY(pttable, map.PT_index);

    if (!IS_PRESENT(ptentry))
    {
        panic("[DEBUG][PT] pttable->entries[PT_index] not present\n");
    }

    return (void*)(get_pdpp(ptentry, PAGE_SIZE_4KIB) | ((uint64_t)virtual_address & 0xfff));
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

    memset(addr, 0x42, PAGE_SIZE_4KIB);
    kprintf("Value at 0x%llx: 0x%llx\n", addr, *((uint64_t*)addr));

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
            vm_entry* entry = GET_ENTRY(pml4, i);
            vm_entry* new_entry = GET_ENTRY(new_pml4, i);

            if (IS_PRESENT(entry))
            {
                new_entry->directory.P = entry->directory.P;
                new_entry->directory.RW = entry->directory.RW;
                new_entry->directory.US = entry->directory.US;
                new_entry->directory.PWT = entry->directory.PWT;
                new_entry->directory.PCD = entry->directory.PCD;
                new_entry->directory.A = entry->directory.A;
                new_entry->directory.IGNORED1 = entry->directory.IGNORED1;
                new_entry->directory.PS = entry->directory.PS;
                new_entry->directory.IGNORED2 = entry->directory.IGNORED2;
                new_entry->directory.R = entry->directory.R;
                new_entry->directory.PDPP = entry->directory.PDPP;
                new_entry->directory.RESERVED = entry->directory.RESERVED;
                new_entry->directory.IGNORED3 = entry->directory.IGNORED3;
                new_entry->directory.XD = entry->directory.XD;
            
                duplicate_page_directory(
                    (struct page_directory*)((uint64_t)entry->directory.PDPP << 12),
                    (struct page_directory*)((uint64_t)new_entry->directory.PDPP << 12),
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

void set_permissions(vm_entry* entry, uint8_t flags)
{
    entry->directory.RW = WRITE_BIT_SET(flags);
    entry->directory.US = USER_BIT_SET(flags);
    entry->directory.PCD = CACHE_BIT_SET(flags);
    entry->directory.XD = NX_BIT_SET(flags);    
}

uint64_t mprotect_page(struct page_directory * root, void* address, uint8_t flags)
{
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    kprintf("Mprotecting vaddr: %llx (phys: %llx) with flags: %x\n", address, get_physical_address(root, address), flags);

    struct page_directory* pdptable, *pdtable, *pttable;
    vm_entry *pml4entry, *pdptentry, *pdentry, *ptentry;

    pml4entry = GET_ENTRY(root, map.PML4_index);
    if (!IS_PRESENT(pml4entry))
    {
        return 0;
    }

    pdptable = (struct page_directory*)get_pdpp(pml4entry, PAGE_SIZE_DIR);
    pdptentry = GET_ENTRY(pdptable, map.PDP_index);

    if (!IS_PRESENT(pdptentry))
    {
        return 0;
    }
    
    if (pdptentry->huge.PS)
    {
        set_permissions(pdptentry, flags);
        if (USER_BIT_SET(flags))
        {
            pml4entry->directory.US = 1;
            pdptentry->huge.US = 1;
        }
        return PAGE_SIZE_1GIB;
    }

    pdtable = (struct page_directory*)get_pdpp(pdptentry, PAGE_SIZE_DIR);
    pdentry = GET_ENTRY(pdtable, map.PD_index);

    if (!IS_PRESENT(pdentry))
    {
        return 0;
    }
    if (pdptentry->big.PS)
    {
        set_permissions(pdentry, flags);
        if (USER_BIT_SET(flags))
        {
            pml4entry->directory.US = 1;
            pdptentry->directory.US = 1;
            pdentry->big.US = 1;
        }
        return PAGE_SIZE_2MIB;
    }

    pttable = (struct page_directory*)get_pdpp(pdentry, PAGE_SIZE_DIR);
    ptentry = GET_ENTRY(pttable, map.PT_index);

    if (!IS_PRESENT(ptentry))
    {
        return 0;
    }

    set_permissions(ptentry, flags);
    if (USER_BIT_SET(flags))
    {
        pml4entry->directory.US = 1;
        pdptentry->directory.US = 1;
        pdentry->directory.US = 1;
        ptentry->regular.US = 1;
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

uint8_t get_page_perms(struct page_directory *pml4, void* address)
{
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory* pdptable, *pdtable, *pttable;
    vm_entry *pml4entry, *pdptentry, *pdentry, *ptentry;

    pml4entry = GET_ENTRY(pml4, map.PML4_index);
    if (!IS_PRESENT(pml4entry))
    {
        return 0;
    }

    pdptable = (struct page_directory*)get_pdpp(pml4entry, PAGE_SIZE_DIR);
    pdptentry = GET_ENTRY(pdptable, map.PDP_index);

    if (!IS_PRESENT(pdptentry))
    {
        return 0;
    }
    
    if (pdptentry->huge.PS)
    {
        uint8_t user_bit = (pml4entry->directory.US & pdptentry->huge.US);
        return pdptentry->huge.RW | (user_bit << 1) | (pdptentry->huge.XD << 2) | (pdptentry->huge.PCD << 3);
    }

    pdtable = (struct page_directory*)get_pdpp(pdptentry, PAGE_SIZE_DIR);
    pdentry = GET_ENTRY(pdtable, map.PD_index);

    if (!IS_PRESENT(pdentry))
    {
        return 0;
    }

    if (pdptentry->big.PS)
    {
        uint8_t user_bit = (pml4entry->directory.US & pdptentry->directory.US & pdentry->big.US);
        return pdentry->big.RW | (user_bit << 1) | (pdentry->big.XD << 2) | (pdentry->big.PCD << 3);
    }

    pttable = (struct page_directory*)get_pdpp(pdentry, PAGE_SIZE_DIR);
    ptentry = GET_ENTRY(pttable, map.PT_index);

    if (!IS_PRESENT(ptentry))
    {
        return 0;
    }

    uint8_t user_bit = (pml4entry->directory.US & pdptentry->directory.US & pdentry->directory.US & ptentry->regular.US);
    return ptentry->regular.RW | (user_bit << 1) | (ptentry->regular.XD << 2) | (ptentry->regular.PCD << 3);
}

uint8_t is_user_access(struct page_directory* pml4, void * address)
{
    return USER_BIT_SET(get_page_perms(pml4, address));
}

uint8_t is_present(struct page_directory* pml4, void * address)
{
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory* pdptable, *pdtable, *pttable;
    vm_entry *pml4entry, *pdptentry, *pdentry, *ptentry;

    pml4entry = GET_ENTRY(pml4, map.PML4_index);
    if (!IS_PRESENT(pml4entry))
    {
        return 0;
    }

    pdptable = (struct page_directory*)get_pdpp(pml4entry, PAGE_SIZE_DIR);
    pdptentry = GET_ENTRY(pdptable, map.PDP_index);

    if (!IS_PRESENT(pdptentry))
    {
        return 0;
    }
    
    if (pdptentry->huge.PS)
    {
        return 1;
    }

    pdtable = (struct page_directory*)get_pdpp(pdptentry, PAGE_SIZE_DIR);
    pdentry = GET_ENTRY(pdtable, map.PD_index);

    if (!IS_PRESENT(pdentry))
    {
        return 0;
    }

    if (pdptentry->big.PS)
    {
        return 1;
    }

    pttable = (struct page_directory*)get_pdpp(pdentry, PAGE_SIZE_DIR);
    ptentry = GET_ENTRY(pttable, map.PT_index);

    if (!IS_PRESENT(ptentry))
    {
        return 0;
    }

    return 1;
}

uint8_t is_writeable(struct page_directory* pml4, void * address)
{
    return WRITE_BIT_SET(get_page_perms(pml4, address));
}

uint8_t is_executable(struct page_directory* pml4, void * address)
{
    return NX_BIT_SET(get_page_perms(pml4, address));
}

void print_entry(vm_entry* entry, uint64_t size)
{
    kprintf("DUMPING ENTRY: 0x%llx\n", entry);
    kprintf("\tP: %d RW: %d US: %d PWT: %d PCD: %d A: %d IGNORED1: %d \n", 
        entry->directory.P,
        entry->directory.RW,
        entry->directory.US,
        entry->directory.PWT,
        entry->directory.PCD,
        entry->directory.A,
        entry->directory.IGNORED1
    );
    kprintf("\tIGNORED2: %d R: %d RESERVED: %d IGNORED3: %d XD: %d\n",
        entry->directory.IGNORED2,
        entry->directory.R,
        entry->directory.RESERVED,
        entry->directory.IGNORED3,
        entry->directory.XD
    );

    switch (size)
    {
        case PAGE_SIZE_1GIB:
            kprintf("\t\t[HUGE ENTRY] PS: %d PDPP: %llx\n", entry->huge.PS, entry->huge.PDPP);
            kprintf("\t\tPhys addr: 0x%llx\n", entry->huge.PDPP);
            break;
        case PAGE_SIZE_2MIB:
            kprintf("\t\t[BIG ENTRY] PS: %d PDPP: %llx\n", entry->big.PS, entry->big.PDPP);
            kprintf("\t\tPhys addr: 0x%llx\n", entry->big.PDPP);
            break;
        case PAGE_SIZE_4KIB:
            kprintf("\t\t[REGULAR ENTRY] PDPP: %llx\n", entry->regular.PDPP);
            kprintf("\t\tPhys addr: 0x%llx\n", entry->regular.PDPP);
            break;
        default:
            kprintf("\t\t[DIRECTORY ENTRY] PS: %d PDPP: %llx\n", entry->directory.PS, entry->directory.PDPP);
            kprintf("\t\tPhys addr: 0x%llx\n", entry->directory.PDPP);
            break;
    }
}

void debug_address(struct page_directory * pml4, void * address)
{
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory* pdptable, *pdtable, *pttable;
    vm_entry *pml4entry, *pdptentry, *pdentry, *ptentry;

    pml4entry = GET_ENTRY(pml4, map.PML4_index);
    print_entry(pml4entry, PAGE_SIZE_DIR);
    if (!IS_PRESENT(pml4entry))
    {
        kprintf("PML4 entry not present\n");
        return;
    }
    
    pdptable = (struct page_directory*)get_pdpp(pml4entry, PAGE_SIZE_DIR);
    pdptentry = GET_ENTRY(pdptable, map.PDP_index);

    if (!IS_PRESENT(pdptentry))
    {
        kprintf("PDPT entry not present\n");
        print_entry(pdptentry, PAGE_SIZE_DIR);
        return;
    }

    if (pdptentry->huge.PS)
    {
        kprintf("PDPT entry is a 1GiB page\n");
        print_entry(pdptentry, PAGE_SIZE_DIR);
        return;
    } else {
        kprintf("PDPT entry is a directory\n");
        print_entry(pdptentry, PAGE_SIZE_DIR);
    }

    pdtable = (struct page_directory*)get_pdpp(pdptentry, PAGE_SIZE_DIR);
    pdentry = GET_ENTRY(pdtable, map.PD_index);
    
    if (!IS_PRESENT(pdentry))
    {
        kprintf("PD entry not present\n");
        print_entry(pdentry, PAGE_SIZE_DIR);
        return;
    }

    if (pdptentry->big.PS)
    {
        kprintf("PD entry is a 2MiB page\n");
        print_entry(pdentry, PAGE_SIZE_DIR);
        return;
    } else {
        kprintf("PD entry is a directory\n");
        print_entry(pdentry, PAGE_SIZE_DIR);
    }

    pttable = (struct page_directory*)get_pdpp(pdentry, PAGE_SIZE_DIR);
    ptentry = GET_ENTRY(pttable, map.PT_index);

    if (!IS_PRESENT(ptentry))
    {
        kprintf("PT entry not present\n");
        print_entry(ptentry, PAGE_SIZE_DIR);
        return;
    }

    kprintf("PT entry is a 4KiB page\n");
    print_entry(ptentry, PAGE_SIZE_4KIB);
}