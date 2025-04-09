#include <omen/libraries/std/stdint.h>
#include <omen/apps/debug/debug.h>
#include <omen/managers/mem/pmm.h>
#include <omen/managers/mem/vmm.h>
#include <omen/apps/panic/panic.h>
#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/libraries/std/string.h>
#include <generic/config.h>

//Memory layout
//0xFFFFA00000000000 - 0xFFFFA0EFFFFFFFFF Identity map for kernel use
//0xFFFFB00000000000 - 0xFFFFB0EFFFFFFFFF Kernel stack
//------------------------------------------------------
//0x0000B00000000000 - 0x0000B0EFFFFFFFFF User space stack
//0x0000C00000000000 - 0x0000C0EFFFFFFFFF User space shared memory and mmaps
//0x0000D00000000000 - 0x0000D0EFFFFFFFFF User space heap

#define PHYSICAL_MEMORY_SIZE    0x000000F000000000
uint64_t physical_memory_offset = 0;

#define IS_PRESENT(entry) ((entry)->directory.P)
#define IS_WRITEABLE(entry) ((entry)->directory.RW)
#define IS_USER_ACCESS(entry) ((entry)->directory.US)
#define IS_EXECUTABLE(entry) (!((entry)->directory.XD))

#define GET_PDPP_HUGE(entry) ((uint64_t)(((uint64_t)(entry)->huge.PDPP) << 30))
#define GET_PDPP_BIG(entry) ((uint64_t)(((uint64_t)(entry)->big.PDPP) << 21))
#define GET_PDPP_REGULAR(entry) ((uint64_t)(((uint64_t)(entry)->regular.PDPP) << 12))
#define GET_PDPP_DIR(entry) ((uint64_t)(((uint64_t)(entry)->directory.PDPP) << 12))
#define GET_ENTRY(root, index) (vm_entry*)(&((root)->entries[(index)]))

void vmm_entry_to_perms(vm_entry * entry, vmm_perms * perms) {
    perms->read_write = entry->directory.RW;
    perms->user = entry->directory.US;
    perms->write_through = entry->directory.PWT;
    perms->cache_disable = entry->directory.PCD;
    perms->global = entry->huge.G;
    perms->no_execute = entry->directory.XD;
}

void flags_to_perms(uint8_t flags, vmm_perms * perms) {
    //Flags format:
    // 0x1 - Read/Write
    // 0x2 - User/Supervisor
    // 0x4 - Write-Through
    // 0x8 - Cache Disable
    // 0x10 - Global
    // 0x20 - No Execute

    perms->read_write = VMM_WRITE_BIT_SET(flags) ? 1 : 0;
    perms->user = VMM_USER_BIT_SET(flags) ? 1 : 0;
    perms->write_through = (VMM_WRITE_THROUGH_BIT_SET(flags)) ? 1 : 0;
    perms->cache_disable = (VMM_CACHE_DISABLE_BIT_SET(flags)) ? 1 : 0;
    perms->global = (VMM_GLOBAL_BIT_SET(flags)) ? 1 : 0;
    perms->no_execute = (VMM_NX_BIT_SET(flags)) ? 1 : 0;
}

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
            return TO_IDENTITY_MAP(GET_PDPP_HUGE(entry));
        case PAGE_SIZE_2MIB:
            return TO_IDENTITY_MAP(GET_PDPP_BIG(entry));
        case PAGE_SIZE_4KIB:
            return TO_IDENTITY_MAP(GET_PDPP_REGULAR(entry));
        default:
            return TO_IDENTITY_MAP(GET_PDPP_DIR(entry));
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
    return TO_IDENTITY_MAP(cr3);
}

void init_entry(vm_entry * entry, uint64_t size, uint64_t page_ppn, vmm_perms perms)
{   

    //Check that page_ppn is aligned to the size, else panic
    if (size == PAGE_SIZE_1GIB && (page_ppn & 0x3fffffff) != 0)
    {
        panic("Page ppn is not aligned to 1GiB page size");
    }

    if (size == PAGE_SIZE_2MIB && (page_ppn & 0x1fffff) != 0)
    {
        panic("Page ppn is not aligned to 2MiB page size");
    }

    if (size == PAGE_SIZE_4KIB && (page_ppn & 0xfff) != 0)
    {
        panic("Page ppn is not aligned to 4KiB page size");
    }

    entry->directory.P = 1;          //0
    entry->directory.RW = perms.read_write; //1
    entry->directory.US = perms.user; //2
    entry->directory.PWT = perms.write_through; //3
    entry->directory.PCD = perms.cache_disable; //4
    entry->directory.A = 0;          //5
    entry->directory.IGNORED1 = 0;          //6
    entry->directory.PS = 0;         //7
    entry->directory.IGNORED2 = 0;   //8-10
    entry->directory.R = 0;          //11
    entry->directory.PDPP = 0;         //12-39 Provisionally set to 0
    entry->directory.RESERVED = 0;   //40-51
    entry->directory.IGNORED3 = 0;   //52-62
    entry->directory.XD = perms.no_execute; //63

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

void map_address(struct page_directory* root, void * virtual_address, void * physical_address, uint64_t size, uint8_t flags)
{
    struct page_map_index map;
    address_to_map((uint64_t)virtual_address, &map);

    struct page_directory *pdptable, *pdtable, *pttable;
    vm_entry *pml4entry, *pdptentry, *pdentry, *ptentry;

    vmm_perms perms;
    perms.read_write = 1;
    perms.user = 1;
    perms.write_through = 0;
    perms.cache_disable = 0;
    perms.global = 0;
    perms.no_execute = 0;

    vmm_perms page_perms;
    flags_to_perms(flags, &page_perms);

    pml4entry = GET_ENTRY(root, map.PML4_index);
    if (!IS_PRESENT(pml4entry)) {
        init_entry(pml4entry, PAGE_SIZE_DIR, (uint64_t)allocate_phys_page(), perms);
    }

    pdptable = (struct page_directory*)get_pdpp(pml4entry, PAGE_SIZE_DIR);
    pdptentry = GET_ENTRY(pdptable, map.PDP_index);

    if (!IS_PRESENT(pdptentry)) {
        if (size == PAGE_SIZE_1GIB) {
            init_entry(pdptentry, PAGE_SIZE_1GIB, (uint64_t)physical_address, page_perms);
            flush_tlb_entry(virtual_address);
            goto check_mapping;
        } else {
            init_entry(pdptentry, PAGE_SIZE_DIR, (uint64_t)allocate_phys_page(), perms);
        }
    } else if (pdptentry->huge.PS) {
        if (size == PAGE_SIZE_1GIB) {
            init_entry(pdptentry, PAGE_SIZE_1GIB, (uint64_t)physical_address, page_perms);
            flush_tlb_entry(virtual_address);
            kprintf("1g Mapping overlap detected, trying to map: %llx, already mapped: %llx\n", virtual_address, get_pdpp(pdptentry, PAGE_SIZE_1GIB));
            goto check_mapping;
        } else {
            kprintf("%llx 1g Mapping overlap detected, trying to map: %llx, already mapped: %llx\n", virtual_address, physical_address, get_pdpp(pdptentry, PAGE_SIZE_1GIB));
            panic("1GiB Mapping overlap detected\n");
        }
    }

    pdtable = (struct page_directory*)get_pdpp(pdptentry, PAGE_SIZE_DIR);
    pdentry = GET_ENTRY(pdtable, map.PD_index);

    if (!IS_PRESENT(pdentry)) {
        if (size == PAGE_SIZE_2MIB) {
            init_entry(pdentry, PAGE_SIZE_2MIB, (uint64_t)physical_address, page_perms);
            flush_tlb_entry(virtual_address);
            goto check_mapping;
        } else {
            init_entry(pdentry, PAGE_SIZE_DIR, (uint64_t)allocate_phys_page(), perms);
        }
    } else if (pdentry->big.PS) {
        if (size == PAGE_SIZE_2MIB) {
            init_entry(pdentry, PAGE_SIZE_2MIB, (uint64_t)physical_address, page_perms);
            flush_tlb_entry(virtual_address);
            kprintf("2m Mapping overlap detected, trying to map: %llx, already mapped: %llx\n", virtual_address, get_pdpp(pdentry, PAGE_SIZE_2MIB));
            goto check_mapping;
        } else {
            kprintf("2m Mapping overlap detected, trying to map: %llx, already mapped: %llx\n", virtual_address, physical_address);
            panic("2MiB Mapping overlap detected\n");
        }
    }

    pttable = (struct page_directory*)get_pdpp(pdentry, PAGE_SIZE_DIR);
    ptentry = GET_ENTRY(pttable, map.PT_index);

    if (!IS_PRESENT(ptentry)) {
        init_entry(ptentry, PAGE_SIZE_4KIB, (uint64_t)physical_address, page_perms);
        flush_tlb_entry(virtual_address);
    } else {
        kprintf("4k Mapping overlap detected, trying to map: %llx, already mapped: %llx\n", virtual_address, physical_address);
        init_entry(ptentry, PAGE_SIZE_4KIB, (uint64_t)physical_address, page_perms);
        flush_tlb_entry(virtual_address);
    }

check_mapping:
    //Print virtual address
    void * phys_addr = get_physical_address(root, virtual_address);
    //Check that the address is correct
    if (phys_addr != physical_address)
    {
        kprintf("Mapping error: %llx != %llx\n", phys_addr, physical_address);
        panic("Mapping error");
    }
}

void unmap_memory(struct page_directory* root, void* virtual_address)
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
        panic("Cant unmap 1GiB page\n");
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
        panic("Cant unmap 2MiB page\n");
    }

    pttable = (struct page_directory*)get_pdpp(pdentry, PAGE_SIZE_DIR);
    ptentry = GET_ENTRY(pttable, map.PT_index);

    if (!IS_PRESENT(ptentry))
    {
        return;
    }

    ptentry->directory.P = 0;
}

void * allocate_vmm(struct page_directory * pml4, uint64_t size, uint64_t region, uint8_t flags)
{
    if (region != VMM_REGION_U_STACK && region != VMM_REGION_U_HEAP && region != VMM_REGION_U_SHM_MMAP && region != VMM_REGION_K_STACK && region != VMM_REGION_K_IDENT && region != VMM_REGION_DEVICES && region != VMM_REGION_K_HEAP)
    {
        panic("Invalid region for allocation\n");
        return NULL;
    }

    void * buffer = pmm_alloc(size);
    if (buffer == NULL)
    {
        panic("Failed to allocate memory\n");
        return NULL;
    }

    memset(TO_IDENTITY_MAP(buffer), 0, size);
    if (region == VMM_REGION_K_IDENT)
    {
        return TO_IDENTITY_MAP(buffer);
    }

    void * vaddr = (void*)((uint64_t)region + (uint64_t)buffer);
    map_range(pml4, vaddr, buffer, PAGE_SIZE_4KIB, size, flags);

    return vaddr;
}

void free_vmm(struct page_directory * pml4, void * address)
{
    if ((uint64_t)address > VMM_REGION_K_IDENT && (uint64_t)address < VMM_REGION_K_IDENT + PHYSICAL_MEMORY_SIZE)
    {
        //kprintf("[DEBUG] Address is in the identity map, freeing physical address\n");
        pmm_free((void*)((uint64_t)address - VMM_REGION_K_IDENT));
        return;
    }

    void * physical = get_physical_address(pml4, address);
    pmm_free(physical);
    unmap_memory(pml4, address);
}

void map_range(struct page_directory* root, void * virtual_start, void * physical_start, uint64_t page_size, uint64_t size, uint8_t flags)
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
        map_address(root, (void*)((uint64_t)virtual_start + (i * page_size)), (void*)((uint64_t)physical_start + (i * page_size)), page_size, flags);
    }

    kprintf("Mapped range from 0x%llx to 0x%llx\n", virtual_start, (uint64_t)virtual_start + size);
}

void duplicate_page_directory(struct page_directory* root, struct page_directory* new, uint8_t level, int override, uint8_t root_on_phys)
{
    for (int i = override; i < 512; i++)
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
                new_entry->directory.PDPP = ((uint64_t)allocate_phys_page()) >> 12;

                uint64_t source_pdpp = (root_on_phys) ? GET_PDPP_DIR(entry) : get_pdpp(entry, PAGE_SIZE_DIR);

                duplicate_page_directory(
                    (struct page_directory*)source_pdpp,
                    (struct page_directory*)get_pdpp(new_entry, PAGE_SIZE_DIR),
                    level - 1,
                    0,
                    root_on_phys
                );
            }
        }
    }
}

struct page_directory * vmm_copy_kernel(struct page_directory* root)
{
    struct page_directory* new = (struct page_directory*)TO_IDENTITY_MAP(allocate_phys_page());
    duplicate_page_directory(root, new, 3, 256, 0);
    return new;
}

struct page_directory * vmm_copy(struct page_directory* root)
{
    struct page_directory* new = (struct page_directory*)TO_IDENTITY_MAP(allocate_phys_page());
    duplicate_page_directory(root, new, 3, 0, 0);
    return new;
}

void compare_directories(struct page_directory* root, struct page_directory* new, uint8_t level)
{
    kprintf("Comparing directories (%llx vs %llx) at level %d\n", root, new, level);
    for (int i = 0; i < 512; i++)
    {
        vm_entry* entry = GET_ENTRY(root, i);
        vm_entry* new_entry = GET_ENTRY(new, i);
        if (IS_PRESENT(entry))
        {
            if (new_entry->directory.P != entry->directory.P)
            {
                kprintf("P bit mismatch at level %d, entry %d\n", level, i);
            }
            if (new_entry->directory.RW != entry->directory.RW)
            {
                kprintf("RW bit mismatch at level %d, entry %d\n", level, i);
            }
            if (new_entry->directory.US != entry->directory.US)
            {
                kprintf("US bit mismatch at level %d, entry %d\n", level, i);
            }
            if (new_entry->directory.PWT != entry->directory.PWT)
            {
                kprintf("PWT bit mismatch at level %d, entry %d\n", level, i);
            }
            if (new_entry->directory.PCD != entry->directory.PCD)
            {
                kprintf("PCD bit mismatch at level %d, entry %d\n", level, i);
            }
            if (new_entry->directory.A != entry->directory.A)
            {
                kprintf("A bit mismatch at level %d, entry %d\n", level, i);
            }
            if (new_entry->directory.IGNORED1 != entry->directory.IGNORED1)
            {
                kprintf("IGNORED1 bit mismatch at level %d, entry %d\n", level, i);
            }
            if (new_entry->directory.PS != entry->directory.PS)
            {
                kprintf("PS bit mismatch at level %d, entry %d\n", level, i);
            }
            if (new_entry->directory.IGNORED2 != entry->directory.IGNORED2)
            {
                kprintf("IGNORED2 bit mismatch at level %d, entry %d\n", level, i);
            }
            if (new_entry->directory.R != entry->directory.R)
            {
                kprintf("R bit mismatch at level %d, entry %d\n", level, i);
            }
            if (new_entry->directory.RESERVED != entry->directory.RESERVED)
            {
                kprintf("RESERVED bit mismatch at level %d, entry %d\n", level, i);
            }
            if (new_entry->directory.IGNORED3 != entry->directory.IGNORED3)
            {
                kprintf("IGNORED3 bit mismatch at level %d, entry %d\n", level, i);
            }
            if (new_entry->directory.XD != entry->directory.XD)
            {
                kprintf("XD bit mismatch at level %d, entry %d\n", level, i);
            }

            if (level > 0 && !entry->directory.PS)
            {
                compare_directories(
                    (struct page_directory*)get_pdpp(entry, PAGE_SIZE_DIR),
                    (struct page_directory*)get_pdpp(new_entry, PAGE_SIZE_DIR),
                    level - 1
                );
            }
        }
    }
}


void* get_physical_address(struct page_directory* root, void* virtual_address)
{

    //Check if address is in the IDENTITIY MAP
    if ((uint64_t)virtual_address > VMM_REGION_K_IDENT && (uint64_t)virtual_address < VMM_REGION_K_IDENT + PHYSICAL_MEMORY_SIZE)
    {
        //kprintf("[DEBUG] Address is in the identity map, returning physical address\n");
        return (void*)((uint64_t)virtual_address - VMM_REGION_K_IDENT);
    }

    struct page_map_index map;
    address_to_map((uint64_t)virtual_address, &map);

    //kprintf("[DEBUG] Virtual address: 0x%llx\n", virtual_address);
    //kprintf("[DEBUG] PML4 index: %d\n", map.PML4_index);
    //kprintf("[DEBUG] PDP index: %d\n", map.PDP_index);
    //kprintf("[DEBUG] PD index: %d\n", map.PD_index);
    //kprintf("[DEBUG] PT index: %d\n", map.PT_index);
    //kprintf("[DEBUG] Offset: %llx\n", (uint64_t)virtual_address & 0xfff);

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
        //kprintf("[DEBUG][PDP] pdptable->entries[PDP_index] is a 1GiB page\n");
        return (void*)((((uint64_t)pdptentry->huge.PDPP) << 30) | ((uint64_t)virtual_address & 0x3fffffff));
    }
    
    pdtable = (struct page_directory*)get_pdpp(pdptentry, PAGE_SIZE_DIR);
    pdentry = GET_ENTRY(pdtable, map.PD_index);

    if (!IS_PRESENT(pdentry))
    {
        panic("[DEBUG][PD] pdtable->entries[PD_index] not present\n");
    } else if (pdentry->big.PS)
    {
        //kprintf("[DEBUG][PD] pdtable->entries[PD_index] is a 2MiB page\n");
        return (void*)((((uint64_t)pdentry->big.PDPP) << 21) | ((uint64_t)virtual_address & 0x1fffff));
    }

    pttable = (struct page_directory*)get_pdpp(pdentry, PAGE_SIZE_DIR);
    ptentry = GET_ENTRY(pttable, map.PT_index);

    if (!IS_PRESENT(ptentry))
    {
        panic("[DEBUG][PT] pttable->entries[PT_index] not present\n");
    }

    return (void*)((((uint64_t)ptentry->regular.PDPP) << 12) | ((uint64_t)virtual_address & 0xfff));
}

void init_vmm()
{
    map_range(get_current_cr3(), (void*)VMM_REGION_K_IDENT, (void*)0, PAGE_SIZE_1GIB, PHYSICAL_MEMORY_SIZE, VMM_WRITE_BIT | VMM_USER_BIT);

    struct page_directory * cr3 = get_current_cr3();
    vm_entry * vme = (vm_entry*)&(cr3->entries[2]);
    vme->directory.P = 1;
    vme->directory.RW = 1;
    vme->directory.US = 1;
    physical_memory_offset = VMM_REGION_K_IDENT;
    remap_bitfield(VMM_REGION_K_IDENT);
    struct page_directory* global_cr3 = vmm_copy_kernel(get_current_cr3());
    //compare_directories(get_current_cr3(), global_cr3, 4);
    switch_cr3(FROM_IDENTITY_MAP(global_cr3));
    kprintf("Page table switched\n");
}

void * vmm_create_kernel_stack(struct page_directory* stack_root, uint64_t stack_pages, uint8_t flags, uint64_t * stack_base) {
        
    void * new_stack_phys = pmm_alloc(stack_pages*0x1000);
    //Init to zero
    memset(TO_IDENTITY_MAP(new_stack_phys), 0, stack_pages*0x1000);
    void * base_address = (void*)((uint64_t)VMM_REGION_K_STACK+(uint64_t)new_stack_phys);
    //CHECK BOUNDS
    if ((uint64_t)base_address < VMM_REGION_K_STACK || (uint64_t)base_address > VMM_REGION_K_STACK+VMM_REGION_SIZE)
    {
        panic("Kernel stack out of bounds\n");
        return NULL;
    }

    map_range(stack_root, base_address, new_stack_phys, PAGE_SIZE_4KIB, stack_pages*PAGE_SIZE_4KIB, flags);
    
    uint64_t stack_top_address = base_address+stack_pages*PAGE_SIZE_4KIB-0x10;
    //If address is not 16-byte aligned, align it by subtracting the difference
    if (stack_top_address % 0x10)
    {
        stack_top_address -= stack_top_address % 0x10;
    }
    stack_top_address -= 0x8;

    *(uint64_t*)stack_base = base_address;
    return (void*)stack_top_address;
}

void * vmm_copy_stack(struct page_directory* stack_root, void * stack_base, uint64_t stack_size, uint8_t flags)
{
    stack_size = (stack_size + 0xfff) & ~0xfff;
    void * new_stack_phys = pmm_alloc(stack_size);

    memcpy(TO_IDENTITY_MAP(new_stack_phys), stack_base, stack_size);
    map_range(stack_root, stack_base, new_stack_phys, PAGE_SIZE_4KIB, stack_size, flags);
    return stack_base;
}

//Adapters for the interface
struct page_directory* get_pml4() {
    return get_current_cr3();
}

uint8_t remap_allocate_cow(struct page_directory * pml4, void * address_raw) {
    panic("Not implemented\n");
    return 0;
}

uint8_t compare_entries(vm_entry* entry1, vm_entry* entry2)
{
    return entry1->directory.P == entry2->directory.P &&
           entry1->directory.RW == entry2->directory.RW &&
           entry1->directory.US == entry2->directory.US &&
           entry1->directory.PWT == entry2->directory.PWT &&
           entry1->directory.PCD == entry2->directory.PCD &&
           entry1->directory.A == entry2->directory.A &&
           entry1->directory.IGNORED1 == entry2->directory.IGNORED1 &&
           entry1->directory.PS == entry2->directory.PS &&
           entry1->directory.IGNORED2 == entry2->directory.IGNORED2 &&
           entry1->directory.R == entry2->directory.R &&
           entry1->directory.RESERVED == entry2->directory.RESERVED &&
           entry1->directory.IGNORED3 == entry2->directory.IGNORED3 &&
           entry1->directory.XD == entry2->directory.XD;
}

void map_memory(struct page_directory * pml4, void * address, void * physical, uint64_t page_size, uint8_t flags) {
    map_address(pml4, address, physical, page_size, flags);
}

void mprotect_current(void* address, uint64_t size, uint8_t flags) {
    mprotect(get_current_cr3(), address, size, flags);
}

void map_current_memory(void * address, void * physical, uint64_t page_size, uint8_t flags) {
    map_memory(get_current_cr3(), address, physical, page_size, flags);
}

void * to_identity_map(void * address) {
    return TO_IDENTITY_MAP(address);
}

void * from_identity_map(void * address) {
    return FROM_IDENTITY_MAP(address);
}

void * get_current_physical_address(void * address) {
    return get_physical_address(get_current_cr3(), address);
}

//Only internal use
void set_pml4(struct page_directory* pml4) {
    switch_cr3(FROM_IDENTITY_MAP(pml4));
}
void * virtual_to_physical(struct page_directory * pml4, void * address) {
    return get_physical_address(pml4, address);
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
    entry->directory.RW = VMM_WRITE_BIT_SET(flags);
    entry->directory.US = VMM_USER_BIT_SET(flags);
    entry->directory.PCD = VMM_CACHE_BIT_SET(flags);
    entry->directory.XD = VMM_NX_BIT_SET(flags);    
}

uint64_t mprotect_page(struct page_directory * root, void* address, uint8_t flags)
{
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    //kprintf("Mprotecting vaddr: %llx (phys: %llx) with flags (W: %d, U: %d, NX: %d, CD: %d)\n", address, get_physical_address(root, address), VMM_WRITE_BIT_SET(flags), VMM_USER_BIT_SET(flags), NX_BIT_SET(flags), CACHE_BIT_SET(flags));

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
        panic("Cant mprotect 1GiB page\n");
        return 0;
    }

    pdtable = (struct page_directory*)get_pdpp(pdptentry, PAGE_SIZE_DIR);
    pdentry = GET_ENTRY(pdtable, map.PD_index);

    if (!IS_PRESENT(pdentry))
    {
        return 0;
    }
    if (pdptentry->big.PS)
    {
        panic("Cant mprotect 2MiB page\n");
        return 0;
    }

    pttable = (struct page_directory*)get_pdpp(pdentry, PAGE_SIZE_DIR);
    ptentry = GET_ENTRY(pttable, map.PT_index);

    if (!IS_PRESENT(ptentry))
    {
        return 0;
    }

    set_permissions(ptentry, flags);
    if (VMM_USER_BIT_SET(flags))
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
    return VMM_USER_BIT_SET(get_page_perms(pml4, address));
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
    return VMM_WRITE_BIT_SET(get_page_perms(pml4, address));
}

uint8_t is_executable(struct page_directory* pml4, void * address)
{
    return VMM_NX_BIT_SET(get_page_perms(pml4, address));
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

void vmm_unmap_userspace(struct page_directory* root)
{
    for (uint64_t i = 0; i < 256; i++)
    {
        memset(GET_ENTRY(root, i), 0, sizeof(vm_entry)); //TODO: Reclaim the used memory!
    }
}

void debug_current_address(void * address)
{
    debug_address(get_current_cr3(), address);
}

void * allocate_phys_page() {
    void * buffer = pmm_alloc_page();
    if (buffer == NULL)
    {
        return NULL;
    }
    memset(TO_IDENTITY_MAP(buffer), 0, PAGE_SIZE_4KIB);
    return buffer;
}

void * allocate_at_vaddr(struct page_directory * root, void * virtual_address, uint64_t pages, uint8_t flags)
{
    void * buffer = pmm_alloc(pages * PAGE_SIZE_4KIB);
    if (buffer == NULL)
    {
        return NULL;
    }
    memset(TO_IDENTITY_MAP(buffer), 0, pages * PAGE_SIZE_4KIB);
    map_memory(root, virtual_address, buffer, PAGE_SIZE_4KIB, flags);
    return buffer;
}

void * get_contiguous_free_range(struct page_directory * root, void * start_addres, uint64_t pages) {
    uint64_t start = (uint64_t)start_addres;
    uint64_t end = start + pages * PAGE_SIZE_4KIB;

    for (uint64_t i = start; i < end; i += PAGE_SIZE_4KIB)
    {
        if (get_physical_address(root, (void*)i) != NULL)
        {
            return NULL;
        }
    }

    return (void*)start;
}

void * get_shm_vmaddress(struct page_directory * root, void * hint, uint64_t size) {
    if (hint == 0) hint = VMM_REGION_U_SHM_MMAP;
    
    uint64_t shm_address = hint;
    uint64_t pages = size / PAGE_SIZE_4KIB;
    if (size % PAGE_SIZE_4KIB)
    {
        pages++;
    }

    while (shm_address < (uint64_t)hint + VMM_REGION_SIZE)
    {
        void * address = get_contiguous_free_range(root, (void*)shm_address, pages);
        if (address != NULL)
        {
            return address;
        }
        shm_address += PAGE_SIZE_4KIB;
    }
    panic("No free memory for shared memory\n");
}

void free_phys_page(void * address) {
    pmm_free(address);
}