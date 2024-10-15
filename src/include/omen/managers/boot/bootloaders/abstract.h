#ifndef _ABSTRACT_BOOTLOADER_H
#define _ABSTRACT_BOOTLOADER_H

#include <omen/libraries/std/stdint.h>

struct bootloader_operations {
    void * (*get_terminal_writer)();
    char * (*get_bootloader_name)();
    char * (*get_bootloader_version)();
    uint64_t (*get_terminal_count)();
    uint64_t (*get_current_terminal)();
    int64_t  (*get_boot_time)();
    uint64_t (*get_memory_map_entries)();
    uint64_t (*get_memory_map_base)(uint64_t);
    uint64_t (*get_memory_map_length)(uint64_t);
    uint64_t (*get_memory_map_type)(uint64_t);
    uint64_t (*get_kernel_address_physical)();
    uint64_t (*get_kernel_address_virtual)();
    uint64_t (*get_hhdm_address)();
    uint64_t (*get_rsdp_address)();
    uint64_t (*get_smbios32_address)();
    uint64_t (*get_smbios64_address)();
    uint32_t (*get_smp_flags)();
    uint32_t (*get_smp_bsp_lapic_id)();
    uint64_t (*get_smp_cpu_count)();
    void * (*get_smp_cpus)();
    uint64_t (*get_framebuffer_count)();
    void * (*get_framebuffers)();
    void   (*set_terminal_extra_handler)();
    void   (*set_terminal_writer)(uint64_t);
};

#endif