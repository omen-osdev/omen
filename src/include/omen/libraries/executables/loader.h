#ifndef _LOADER_H
#define _LOADER_H

#include <omen/managers/mem/vmm.h>
#include <omen/libraries/std/stdint.h>

#define AT_NULL         0
#define AT_IGNORE       1
#define AT_EXECFD       2
#define AT_PHDR         3       /* &phdr[0] */
#define AT_PHENT        4       /* sizeof(phdr[0]) */
#define AT_PHNUM        5       /* # phdr entries */
#define AT_PAGESZ       6       /* getpagesize(2) */
#define AT_BASE         7       /* ld.so base addr */
#define AT_FLAGS        8       /* processor flags */
#define AT_ENTRY        9       /* a.out entry point */
#define AT_SYSINFO_EHDR  10      /* sysinfo page */

struct proc_ld {
    void* at_phdr;
    char* ld_path;
};

struct auxv{
    uint64_t a_type;
    void* a_val;
};

struct loaded_elf {
    uint64_t entry;
    struct auxv * auxv;
    uint64_t auxv_size;
    struct proc_ld * ld;
    uint64_t ld_size;
};

char * get_auxv_string(uint64_t type);
void elf_readelf(uint8_t * buffer, uint64_t size);
struct loaded_elf* elf_load_elf(struct page_directory* root, uint8_t * buffer, uint64_t size);
#endif