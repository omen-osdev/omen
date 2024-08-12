#include <generic/config.h>

#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/managers/boot/bootloaders/abstract.h>
#include <omen/libraries/std/stddef.h>

#ifdef BOOTLOADER_USE_LIMINE
#include <omen/managers/boot/bootloaders/limine/bootservices.h>
#endif

struct bootloader_operations * bootloader_operations;

void init_bootloader() {
    bootloader_operations = get_boot_ops();
}

void (*get_terminal_writer())(const char*, uint64_t) {
    void* writer = bootloader_operations->get_terminal_writer();
    return (void (*)(const char*, uint64_t))writer;
}

char* get_bootloader_name() {
    return bootloader_operations->get_bootloader_name();
}

char* get_bootloader_version() {
    return bootloader_operations->get_bootloader_version();
}

uint64_t get_terminal_count() {
    return bootloader_operations->get_terminal_count();
}

uint64_t get_current_terminal() {
    return bootloader_operations->get_current_terminal();
}

int64_t get_boot_time() {
    return bootloader_operations->get_boot_time();
}

uint64_t get_memory_map_entries() {
    return bootloader_operations->get_memory_map_entries();
}

uint64_t get_memory_map_base(uint64_t index) {
    return bootloader_operations->get_memory_map_base(index);
}

uint64_t get_memory_map_length(uint64_t index) {
    return bootloader_operations->get_memory_map_length(index);
}

uint64_t get_memory_map_type(uint64_t index) {
    return bootloader_operations->get_memory_map_type(index);
}

uint64_t get_kernel_address_physical() {
    return bootloader_operations->get_kernel_address_physical();
}

uint64_t get_kernel_address_virtual() {
    return bootloader_operations->get_kernel_address_virtual();
}

uint64_t get_rsdp_address() {
    return bootloader_operations->get_rsdp_address();
}

uint64_t get_smbios32_address() {
    return bootloader_operations->get_smbios32_address();
}

uint64_t get_smbios64_address() {
    return bootloader_operations->get_smbios64_address();
}

uint32_t get_smp_flags() {
    return bootloader_operations->get_smp_flags();
}

uint32_t get_smp_bsp_lapic_id() {
    return bootloader_operations->get_smp_bsp_lapic_id();
}

uint64_t get_smp_cpu_count() {
    return bootloader_operations->get_smp_cpu_count();
}

boot_smp_info_t ** get_smp_cpus() {
    return (boot_smp_info_t **)bootloader_operations->get_smp_cpus();
}

uint64_t get_framebuffer_count() {
    return bootloader_operations->get_framebuffer_count();
}

boot_framebuffer_t ** get_framebuffers() {
    return (boot_framebuffer_t **)bootloader_operations->get_framebuffers();
}

void set_terminal_extra_handler() {
    bootloader_operations->set_terminal_extra_handler();
}

void set_terminal_writer(uint64_t terminal) {
    bootloader_operations->set_terminal_writer(terminal);
}