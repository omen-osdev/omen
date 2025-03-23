#ifndef _LOADER_H
#define _LOADER_H

#include <omen/managers/mem/vmm.h>
#include <omen/libraries/std/stdint.h>

struct proc_ld {
    void* at_phdr;
    char* ld_path;
};

struct auxv{
    uint64_t a_type;
    void* a_val;
};


void elf_readelf(uint8_t * buffer, uint64_t size);
void* elf_load_elf(struct page_directory* root, uint8_t * buffer, uint64_t size);
#endif