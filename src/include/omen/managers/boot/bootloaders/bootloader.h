#ifndef _BOOTLOADER_H
#define _BOOTLOADER_H

#include <generic/config.h>
#include <omen/libraries/std/stdint.h>

#ifdef BOOTLOADER_USE_LIMINE
#include <omen/managers/boot/bootloaders/limine/limine.h>
typedef struct limine_smp_info boot_smp_info_t;
typedef struct limine_video_mode boot_video_mode_t;
typedef struct limine_framebuffer boot_framebuffer_t;
#define BOOTLOADER_MEMMAP_USABLE                 0
#define BOOTLOADER_MEMMAP_RESERVED               1
#define BOOTLOADER_MEMMAP_ACPI_RECLAIMABLE       2
#define BOOTLOADER_MEMMAP_ACPI_NVS               3
#define BOOTLOADER_MEMMAP_BAD_MEMORY             4
#define BOOTLOADER_MEMMAP_BOOTLOADER_RECLAIMABLE 5
#define BOOTLOADER_MEMMAP_KERNEL_AND_MODULES     6
#define BOOTLOADER_MEMMAP_FRAMEBUFFER            7
#endif

void init_bootloader();

void   (*get_terminal_writer())(const char*, uint64_t);
char*    get_bootloader_name();
char*    get_bootloader_version();
uint64_t get_terminal_count();
uint64_t get_current_terminal();
int64_t  get_boot_time();
uint64_t get_memory_map_entries();
uint64_t get_memory_map_base(uint64_t);
uint64_t get_memory_map_length(uint64_t);
uint64_t get_memory_map_type(uint64_t);
uint64_t get_kernel_address_physical();
uint64_t get_kernel_address_virtual();
uint64_t get_rsdp_address();
uint64_t get_smbios32_address();
uint64_t get_smbios64_address();
uint32_t get_smp_flags();
uint32_t get_smp_bsp_lapic_id();
uint64_t get_smp_cpu_count();
boot_smp_info_t ** get_smp_cpus();
uint64_t get_framebuffer_count();
boot_framebuffer_t ** get_framebuffers();
void     set_terminal_extra_handler(); //TODO
void     set_terminal_writer(uint64_t terminal);

#endif