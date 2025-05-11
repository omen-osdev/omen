#ifndef _VMM_ACCOUNT_H
#define _VMM_ACCOUNT_H

#define ACCOUNT_DEBUG_EGGHUNTER 0xffff900100f6b000

struct vmm_mapping {
    void * pd;
    void * virtual_address;
    void * physical_address;
    uint64_t size;
    struct vmm_mapping * next;
};

struct vmm_dir_linked_list {
    void * pd;
    struct vmm_mapping * mappings;
    struct vmm_dir_linked_list * next;
};

void insert_page_directory(void * new);
void remove_page_directory(void *  root);
void copy_page_directory(void * root, void * new_root, void * min_copy_range, void * max_copy_range);
void insert_allocation(void * root, void * virtual_address, void * physical_address, uint64_t size);
void remove_allocation(void * root, void * virtual_address);
uint64_t how_many_allocations_for_vaddr(void * address);
uint64_t how_many_allocations_for_physaddr(void * address);
#endif