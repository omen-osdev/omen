#ifndef _PMM_H
#define _PMM_H

#include <omen/libraries/std/stdint.h>

void pmm_init();
void pmm_list_map();
void * pmm_alloc(uint64_t size);
void pmm_free(void * ptr);

#endif