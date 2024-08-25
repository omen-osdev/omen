#include <omen/libraries/std/stdint.h>
#include <omen/managers/mem/pmm.h>

/**
 * Chronological order of table access to convert virtual address to 
 * physical address for 4kB page size
 * PML4 -> PDPT -> PD -> PT -> Physical base address
 * refer to Intel® 64 and IA-32 Architectures Software Developer’s Manual
 * volume 3, chapter 4
 */
#define PD_LEN 512
#define PT_LEN 512

#define SET_ATTRIBUTE(entry, attr) (entry |= attr)
#define CLEAR_ATTRIBUTE(entry, attr) (entry &= ~attr)
#define HW_ADDRESS_START   ((uint64_t)0xffff800000000000)
#define PMM_BITMAP_START   ((uint64_t)0xffff900000000000)
#define IDENTITY_MAP_START ((uint64_t)0xffffA00000000000)
#define USERLAND_END       ((uint64_t)0x0000800000000000)
#define TO_KERNEL_MAP(vaddr) ((void*)(((uint64_t)(vaddr))+(uint64_t)IDENTITY_MAP_START))
#define FROM_KERNEL_MAP(vaddr) ((void*)(((uint64_t)(vaddr))-(uint64_t)IDENTITY_MAP_START))

typedef uint64_t pd_entry;
typedef uint64_t pt_entry;

typedef enum {
    PDE_PRESENT           = 0x01,                         // 0  bit position
    PDE_READ_WRITE        = 0x02,                         // 1  bit position
    PDE_USER_SUPERVISOR   = 0x04,                         // 2  bit position
    PDE_WRITE_THROUGH     = 0x08,                         // 3  bit position
    PDE_CACHE_DISABLE     = 0x10,                         // 4  bit position
    PDE_ACCESSED          = 0x20,                         // 5  bit position
    PDE_XD                = 0x8000000000000000,           // 63 bit position
} PageDirectoryEntry_Flag;

typedef enum {
    PTE_PRESENT            = 0x01,                         // 0  bit position
    PTE_READ_WRITE         = 0x02,                         // 1  bit position
    PTE_USER_SUPERVISOR    = 0x04,                         // 2  bit position
    PTE_WRITE_THROUGH      = 0x08,                         // 3  bit position
    PTE_CACHE_DISABLE      = 0x10,                         // 4  bit position
    PTE_ACCESSED           = 0x20,                         // 5  bit position
    PTE_XD                 = 0x8000000000000000,           // 63 bit position
} PageTableEntry_Flag;


typedef struct pdTable {
    pd_entry entry[PD_LEN];
} pdTable;

typedef struct ptTable {
    pt_entry entry[PT_LEN];
} ptTable;

typedef struct pageMapIndex {
    uint64_t PML4_idx;
    uint64_t PDPT_idx;
    uint64_t PD_idx;
    uint64_t PT_idx;
} pageMapIndex;

void page_to_map(uint64_t virt_addr, pageMapIndex *map);
void* virt_to_phys(uint64_t virt_addr, pdTable *pml4);
void init();
