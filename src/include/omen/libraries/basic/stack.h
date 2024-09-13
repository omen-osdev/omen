#ifndef _STACK_H
#define _STACK_H

#include <omen/libraries/std/stdint.h>

struct proc_ld {
    void* at_phdr;
    char* ld_path;
};

struct auxv{
    uint64_t a_type;
    void* a_val;
};

#endif