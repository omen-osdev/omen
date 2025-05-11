#include <omen/managers/mem/pmm.h>
#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/managers/mem/account.h>
#include <omen/managers/mem/vmm.h>

struct vmm_dir_linked_list * vmm_dir_list = 0x0;

void insert_page_directory(void* new)
{
    //Search for the page directory in the list
    struct vmm_dir_linked_list * node = vmm_dir_list;
    while (node)
    {
        if (node->pd == new)
        {
            panic("[ACCOUNT] Page directory already present\n");
        }
        node = node->next;
    }
    
    struct vmm_dir_linked_list * node_phys = (struct vmm_dir_linked_list*)pmm_alloc(sizeof(struct vmm_dir_linked_list));
    if (!node_phys)
    {
        panic("Failed to allocate memory for page directory linked list\n");
    }
    node = (struct vmm_dir_linked_list*)to_identity_map(node_phys);
    memset(node, 0, sizeof(struct vmm_dir_linked_list));
    node->pd = new;
    node->mappings = 0x0;
    node->next = vmm_dir_list;
    vmm_dir_list = node;
}

void remove_page_directory(void* root)
{
    struct vmm_dir_linked_list * node = vmm_dir_list;
    struct vmm_dir_linked_list * prev = 0x0;
    while (node)
    {
        if (node->pd == root)
        {
            if (prev)
            {
                prev->next = node->next;
            } else {
                vmm_dir_list = node->next;
            }

            //Free the mappings
            struct vmm_mapping * mapping = node->mappings;
            while (mapping)
            {
                struct vmm_mapping * next = mapping->next;
                pmm_free(from_identity_map(mapping));
                mapping = next;
            }

            pmm_free(from_identity_map(node));
            return;
        }
        prev = node;
        node = node->next;
    }
    panic("[ACCOUNT] Page directory not found\n");
}

void dump_allocations(void* root)
{
    struct vmm_dir_linked_list * node = vmm_dir_list;
    while (node)
    {
        if (node->pd == root)
        {
            struct vmm_mapping * mapping = node->mappings;
            while (mapping)
            {
                kprintf("[ACCOUNT] Mapping: %p -> %p (%d)\n", mapping->virtual_address, mapping->physical_address, mapping->size);
                mapping = mapping->next;
            }
            return;
        }
        node = node->next;
    }
    panic("[ACCOUNT] Page directory not found\n");
}

void insert_allocation(void* root, void * virtual_address, void * physical_address, uint64_t size)
{
    struct vmm_mapping * node_phys = (struct vmm_mapping*)pmm_alloc(sizeof(struct vmm_mapping));
    if (!node_phys)
    {
        panic("Failed to allocate memory for page directory linked list\n");
    }
    struct vmm_mapping * node = (struct vmm_mapping*)to_identity_map(node_phys);
    memset(node, 0, sizeof(struct vmm_mapping));
    node->pd = root;
    node->virtual_address = virtual_address;
    node->physical_address = physical_address;
    node->size = size;
    node->next = 0x0;

#ifdef ACCOUNT_DEBUG_EGGHUNTER
    if (virtual_address == (void*)ACCOUNT_DEBUG_EGGHUNTER)
    {
        kprintf("[ACCOUNT] EggHunter mapping: %p -> %p (%d)\n", virtual_address, physical_address, size);
        BREAKPOINT();
    }
#endif

    struct vmm_dir_linked_list * dir_node = vmm_dir_list;
    while (dir_node)
    {
        if (dir_node->pd == root)
        {
            //Make sure the mapping is not already present
            struct vmm_mapping * node_check = dir_node->mappings;
            while (node_check)
            {
                if (node_check->virtual_address == virtual_address)
                {
                    dump_allocations(root);
                    panic("[ACCOUNT] Mapping already present\n");
                }
                node_check = node_check->next;
            }
            //Insert the new mapping at the end of the list
            if (dir_node->mappings == 0x0)
            {
                dir_node->mappings = node;
            } else {
                struct vmm_mapping * last = dir_node->mappings;
                while (last->next)
                {
                    last = last->next;
                }
                last->next = node;
            }
            return;
        }
        dir_node = dir_node->next;
    }

    if ((uint64_t)root > 0xFFFF000000000000)
        panic("[ACCOUNT] Page directory not found\n");
}

void remove_allocation(void* root, void * virtual_address)
{
    struct vmm_dir_linked_list * node = vmm_dir_list;
    while (node)
    {
        if (node->pd == root)
        {
            struct vmm_mapping * mapping = node->mappings;
            struct vmm_mapping * prev = 0x0;
            while (mapping)
            {
                if (mapping->virtual_address == virtual_address)
                {
                    if (prev)
                    {
                        prev->next = mapping->next;
                    } else {
                        node->mappings = mapping->next;
                    }
                    pmm_free(from_identity_map(mapping));
                }
                prev = mapping;
                mapping = mapping->next;
            }
            return;
        }
        node = node->next;
    }
    panic("[ACCOUNT] Mapping not found\n");
}

uint64_t how_many_allocations_for_vaddr(void * address) {
    struct vmm_dir_linked_list * node = vmm_dir_list;
    uint64_t count = 0;
    while (node)
    {
        struct vmm_mapping * mapping = node->mappings;
        while (mapping)
        {
            //Check if the address is in the range of the mapping
            if (mapping->virtual_address <= address && (mapping->virtual_address + mapping->size) > address)
            {
                count++;
                goto next_mapping;
            }
            mapping = mapping->next;
        }
next_mapping:
        node = node->next;
    }
    return count;
}

uint64_t how_many_allocations_for_physaddr(void * address) {
    struct vmm_dir_linked_list * node = vmm_dir_list;
    uint64_t count = 0;
    while (node)
    {
        struct vmm_mapping * mapping = node->mappings;
        while (mapping)
        {
            //Check if the address is in the range of the mapping
            if (mapping->physical_address <= address && (mapping->physical_address + mapping->size) > address)
            {
                count++;
                goto next_mapping;
            }
            mapping = mapping->next;
        }
next_mapping:
        node = node->next;
    }
    return count;
}

void copy_page_directory(void * root, void * new_root, void * min_copy_range, void * max_copy_range) {
    //Insert the new root in the list
    insert_page_directory(new_root);
    
    //Copy the allocations from the old root to the new root as long as the virtual address is in the range
    struct vmm_dir_linked_list * node = vmm_dir_list;
    while (node)
    {
        if (node->pd == root)
        {
            struct vmm_mapping * mapping = node->mappings;
            while (mapping)
            {
                if (mapping->virtual_address >= min_copy_range && mapping->virtual_address <= max_copy_range)
                {
                    insert_allocation(new_root, mapping->virtual_address, mapping->physical_address, mapping->size);
                }
                mapping = mapping->next;
            }
            return;
        }
        node = node->next;
    }

    panic("[ACCOUNT] Page directory not found\n");
}