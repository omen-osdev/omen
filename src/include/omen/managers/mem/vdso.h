#ifndef _VDSO_H
#define _VDSO_H
#include <omen/managers/mem/vmm.h>

#define VDSO_ENTRY_ID_NONE          0x0
#define VDSO_ENTRY_ID_GETTIMEOFDAY  0x1
#define VDSO_ENTRY_ID_CLOCK_GETTIME 0x2
#define VDSO_ENTRY_SIGNAL_TRAMP     0x3
#define VDSO_ENTRY_SIGNAL_SIGNO     0x4
#define VDSO_ENTRY_SIGNAL_SIGACTION 0x5
#define VDSO_ENTRY_SIGNAL_SIGCTXT   0x6

#define VDSO_REGION_SIZE(x) (-1 * (x))
#define VDSO_IS_REGION(x) (x < 0)

typedef struct vdso_entry {
    uint8_t id;
    void* info;
    int64_t size;
    struct vdso_entry* next;
} vdso_entry_t;


typedef struct vdso {
    struct page_directory* pd;
    uint64_t base_addresss;
    uint64_t size;
    struct vdso_entry* entry;
} __attribute__((packed)) vdso_t;

vdso_t* vdso_init();
int vdso_set_data(vdso_t* vdso, uint8_t id, void* info, int64_t size);
int vdso_get_data(vdso_t* vdso, uint8_t id, void** info, int64_t* size);
void * vdso_allocate_region(vdso_t* vdso, uint64_t size);
void vdso_free_region(vdso_t* vdso, void* region);
vdso_t* get_vdso();
#endif