#ifndef _VMAREA_H
#define _VMAREA_H

#include <omen/libraries/std/stdint.h>
#include <omen/managers/cpu/process.h>

#define VMAREA_CLONE_WITH_COW 0x01

struct vm_area {
    void * start;
    void * end;
    uint8_t flags;
    uint8_t extended_flags;
    uint64_t page_size;
    int fd;
    off_t offset;
    struct vm_area * next;
};

struct vm_area* is_in_vmarea(process_t* process, void * address);
struct vm_area* vmarea_collides(process_t * task, struct vm_area * vma);
void * find_shm_vmarea(process_t * task, void * hint, uint64_t size);
void create_vmarea(process_t* process, void * start, void * end, uint8_t flags, uint8_t extended_flags, uint64_t page_size, int fd, off_t offset);
void remove_vmarea(process_t* process, void * start);
void duplicate_vmareas(process_t * old, process_t * new, uint8_t cow);
void engrave_vmareas(process_t * child, process_t * parent);
void duplicate_vmarea_cow(process_t * task, struct vm_area* vma);
void vmarea_sync(struct vm_area * vma, uint64_t size);
void vmarea_sync_all_files(process_t *task);
void dump_vmareas(process_t* process);
void remove_all_vmareas(process_t * task);
#endif