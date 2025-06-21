#include <vfs/vfs.h>
#include <vfs/vfs_interface.h>
#include <omen/managers/cpu/vmarea.h>
#include <omen/managers/cpu/process.h>
#include <omen/managers/mem/vmm.h>
#include <omen/apps/debug/debug.h>
#include <omen/libraries/allocators/heap_allocator.h>

struct vm_area* is_in_vmarea(process_t* process, void * address) {
    struct vm_area * current = process->vm_areas;
    while (current) {
        if (address >= current->start && address < current->end) {
            return current;
        }
        current = current->next;
    }
    return 0;
}

void dump_vmareas(process_t* process) {
    struct vm_area * current = process->vm_areas;
    while (current) {
        DBG_INFO("[PID: %d | AT: %p | NEXT: %p] VM Area: %p - %p, flags: %x, extended_flags: %x, page_size: %llu, fd: %d, offset: %lld\n", process->pid, current, current->next, current->start, current->end, current->flags, current->extended_flags, current->page_size, current->fd, current->offset);
        current = current->next;
    }
}

void create_vmarea(process_t* process, void * start, void * end, uint8_t flags, uint8_t extended_flags, uint64_t page_size, int fd, off_t offset) {
    struct vm_area * new_area = kmalloc(sizeof(struct vm_area));
    new_area->start = start;
    new_area->end = end;
    new_area->flags = flags;
    new_area->extended_flags = extended_flags;
    new_area->page_size = page_size;
    new_area->next = process->vm_areas;
    new_area->fd = fd;
    new_area->offset = offset;
    process->vm_areas = new_area;
    //DBG_DEBUG("Created VM Area: %p - %p, flags: %x, extended_flags: %x, page_size: %llu, fd: %d, offset: %lld\n", start, end, flags, extended_flags, page_size, fd, offset);
    //dump_vmareas(process);
}

void remove_vmarea(process_t* process, void * start) {
    struct vm_area * current = process->vm_areas;
    struct vm_area * previous = 0;

    while (current) {
        if (current->start == start) {
            if (previous) {
                previous->next = current->next;
            } else {
                process->vm_areas = current->next;
            }
            kfree(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

void duplicate_vmareas(process_t * old, process_t * new, uint8_t cow) {
    struct vm_area * current = old->vm_areas;
    new->vm_areas = 0;
    
    //dump_vmareas(old);
    while (current) {
        if (cow == VMAREA_CLONE_WITH_COW) {
            current->extended_flags |= VMAREA_EXT_COW;
        }

        create_vmarea(new, current->start, current->end, current->flags, current->extended_flags, current->page_size, current->fd, current->offset);
        current = current->next;
    }
    DBG_DEBUG("Duplicated VM Areas from process %d to process %d\n", old->pid, new->pid);
    //dump_vmareas(new);
}

void engrave_vmareas(process_t * child, process_t * parent) {
    DBG_DEBUG("Engraving VM Areas from parent process %d to child process %d\n", parent->pid, child->pid);
    struct vm_area * current = child->vm_areas;
    while (current) {
        //DBG_DEBUG("NEW VMAREA: %p - %p, flags: %x, extended_flags: %x, page_size: %llu, fd: %d, offset: %lld\n", current->start, current->end, current->flags, current->extended_flags, current->page_size, current->fd, current->offset);
        if (current->extended_flags & VMAREA_EXT_SHARED) {
           // DBG_DEBUG("Sharing vma...\n");
            //Map the area in the child process to the same address as the parent
            void * parent_physical = get_physical_address(parent->vmm, current->start);
            map_range(child->vmm, current->start, parent_physical, current->page_size, current->end - current->start, current->flags);
        }
        if ((current->extended_flags & VMAREA_EXT_COW) && !(current->extended_flags & VMAREA_EXT_STACK_GUARD)) {
            //DBG_DEBUG("COW vma...\n");
            uint8_t flags = current->flags;
            if (flags & VMM_WRITE_BIT) {
                flags &= ~VMM_WRITE_BIT;
            }
            mprotect(child->vmm, current->start, current->end - current->start, flags);
            mprotect(parent->vmm, current->start, current->end - current->start, flags);
        }
        current = current->next;
    }
}

void duplicate_vmarea_cow(process_t * task, struct vm_area* vma) {
    remap_allocate_cow(task->vmm, vma->start, vma->end - vma->start, vma->page_size, vma->flags);
    vma->extended_flags &= ~VMAREA_EXT_COW;
}

//Check if vma collides with any other vma in the process
struct vm_area* vmarea_collides(process_t * task, struct vm_area * vma) {
    struct vm_area * current = task->vm_areas;
    while (current) {
        if (current->start < vma->end && vma->start < current->end) {
            return current;
        }
        current = current->next;
    }
    return 0;
}


void * find_shm_vmarea(process_t * task, void * hint, uint64_t size) {
    if (hint == 0) hint = VMM_REGION_U_SHM_MMAP;
    
    struct vm_area desired_vma = {
        .start = hint,
        .end = hint + size,
        .flags = VMM_USER_BIT,
        .extended_flags = VMAREA_EXT_SHARED,
        .page_size = PAGE_SIZE_4KIB,
        .fd = -1,
        .offset = 0,
    };

    struct vm_area * collision = vmarea_collides(task, &desired_vma);
    uint8_t wrapped = 0;
    uint64_t collision_end_aligned;
    while(collision) {
        //Align collision end to next page
        collision_end_aligned = (uint64_t)collision->end;
        collision_end_aligned = (collision_end_aligned + PAGE_SIZE_4KIB - 1) & ~(PAGE_SIZE_4KIB - 1);

        desired_vma.start = collision_end_aligned;
        desired_vma.end = collision_end_aligned + size;
        if ((uint64_t)(desired_vma.start) > VMM_REGION_U_SHM_MMAP + VMM_REGION_SIZE) {
            if (wrapped) {
                return NULL;
            }
            desired_vma.start = VMM_REGION_U_SHM_MMAP;
            desired_vma.end = VMM_REGION_U_SHM_MMAP + size;
            wrapped = 1;
        }
        collision = vmarea_collides(task, &desired_vma);
    }

    return desired_vma.start;
}

void vmarea_sync(struct vm_area * vma, uint64_t size) {

    uint64_t sync_size = (size) ? size : (uint64_t)(vma->end - vma->start);
    if (vma->fd) {
        vfs_file_seek(vma->fd, vma->offset, SEEK_SET);
        vfs_file_write(vma->fd, vma->start, sync_size);
        vfs_file_seek(vma->fd, vma->offset, SEEK_SET);
        vma->extended_flags &= ~VMAREA_EXT_REQ_SYNC;
    }
}

void vmarea_sync_all_files(process_t *task) {
    struct vm_area * current = task->vm_areas;
    while (current) {
        if (current->extended_flags & VMAREA_EXT_REQ_SYNC) { //MAYBE WE NEED TO FORCE THIS INSTEAD OF CHECKING IF REQ_SYNC
            vmarea_sync(current, 0);
        }
        current = current->next;
    }    
}

void remove_all_vmareas(process_t * task) {
    struct vm_area * current = task->vm_areas;
    while (current) {
        struct vm_area * next = current->next;
        kfree(current);
        current = next;
    }
    task->vm_areas = 0;
}