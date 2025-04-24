#ifndef _VDSO_H
#define _VDSO_H

#include <stdint.h>

#define VDSO_ENTRY_ID_NONE          0x0
#define VDSO_ENTRY_ID_GETTIMEOFDAY  0x1
#define VDSO_ENTRY_ID_CLOCK_GETTIME 0x2
#define VDSO_ENTRY_SIGNAL_TRAMP     0x3

typedef struct vdso_entry {
    uint8_t id;
    void* info;
    uint64_t size;
    void* (*act)(struct vdso_entry*);
    struct vdso_entry* next;
} vdso_entry_t;

typedef struct vdso {
    void* pd;
    uint64_t base_addresss;
    uint64_t size;
    struct vdso_entry* entry;
} __attribute__((packed)) vdso_t;

void vdso_set_data(vdso_t* vdso, uint8_t id, void* info, uint64_t size);
void vdso_get_data(vdso_t* vdso, uint8_t id, void** info, uint64_t* size);

void * get_trampoline();
#endif