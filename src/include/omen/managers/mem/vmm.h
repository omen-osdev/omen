/** 
 * This header contains declarations for virtual memory manager
 **/
#include <omen/src/include/omen/libraries/std/stdint.h>
#include <omen/src/include/omen/managers/mem/pmm.h>

/**
 * Chronological order of table access to convert virtual address to physical address for 4kB page size
 * PML4 -> PDPT -> PD -> PT -> Physical base address
 */

// defines number of entry in a particular table
#define PML4_LEN 512
#define PDPT_LEN 512
#define PD_LEN   512
#define PT_LEN   512

#define SET_ATTRIBUTE(entry, attr) (entry |= attr)
#define CLEAR_ATTRIBUTE(entry, attr) (entry &= ~attr)

typedef uint64_t pml4_entry;
typedef uint64_t pdpt_entry;
typedef uint64_t pd_entry;
typedef uint64_t pt_entry;

typedef struct {
    PML4E_PRESENT           = 0x01,                         // 0  bit position
    PML4E_READ_WRITE        = 0x02,                         // 1  bit position
    PML4E_USER_SUPERVISOR   = 0x04,                         // 2  bit position
    PML4E_WRITE_THROUGH     = 0x08,                         // 3  bit position
    PML4E_CACHE_DISABLE     = 0x10,                         // 4  bit position
    PML4E_ACCESSED          = 0x20,                         // 5  bit position
    PML4E_XD                = 0x8000000000000000,           // 63 bit position
} PageMapLevel4Entry_Flag;

typedef struct {
    PDPTE_PRESENT           = 0x01,                         // 0  bit position
    PDPTE_READ_WRITE        = 0x02,                         // 1  bit position
    PDPTE_USER_SUPERVISOR   = 0x04,                         // 2  bit position
    PDPTE_WRITE_THROUGH     = 0x08,                         // 3  bit position
    PDPTE_CACHE_DISABLE     = 0x10,                         // 4  bit position
    PDPTE_ACCESSED          = 0x20,                         // 5  bit position
    PDPTE_XD                = 0x8000000000000000,           // 63 bit position
} PageDirectoryPointerTableEntry_Flag;

typedef struct {
    PDE_PRESENT            = 0x01,                         // 0  bit position
    PDE_READ_WRITE         = 0x02,                         // 1  bit position
    PDE_USER_SUPERVISOR    = 0x04,                         // 2  bit position
    PDE_WRITE_THROUGH      = 0x08,                         // 3  bit position
    PDE_CACHE_DISABLE      = 0x10,                         // 4  bit position
    PDE_ACCESSED           = 0x20,                         // 5  bit position
    PDE_XD                 = 0x8000000000000000,           // 63 bit position
} PageDirectoryEntry_Flag;

typedef struct {
    PTE_PRESENT            = 0x01,                         // 0  bit position
    PTE_READ_WRITE         = 0x02,                         // 1  bit position
    PTE_USER_SUPERVISOR    = 0x04,                         // 2  bit position
    PTE_WRITE_THROUGH      = 0x08,                         // 3  bit position
    PTE_CACHE_DISABLE      = 0x10,                         // 4  bit position
    PTE_ACCESSED           = 0x20,                         // 5  bit position
    PTE_XD                 = 0x8000000000000000,           // 63 bit position
} PageTableEntry_Flag;

// A PML4 table comprises 512 64-bit entries (PML4Es)
pml4_entry PageMapLevel4[PML4_LEN];

// A page-directory-pointer table comprises 512 64-bit entries (PDPTEs)
pdpt_entry PageDirectoryPointerTable[PDPT_LEN];

// A page directory comprises 512 64-bit entries (PDEs)
pd_entry   PageDirectory[PD_LEN];

// A page table comprises 512 64-bit entries (PTEs).
pt_entry   PageTable[PT_LEN];