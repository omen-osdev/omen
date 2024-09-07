#ifndef _PMM_H
#define _PMM_H

#include <omen/libraries/std/stdint.h>

extern uint64_t KERNEL_START;
extern uint64_t KERNEL_END;

struct pmm_block {
    uint64_t base;
    uint64_t size;
    uint64_t type;
};

void pmm_init();
void pmm_list_map();
struct pmm_block * get_main_memory();
void * pmm_alloc(uint64_t size);
void pmm_free(void * ptr);

#endif