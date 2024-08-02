#include <omen/modules/managers/boot/bootloaders/bootloader.h>
#include <omen/modules/managers/boot/bootloaders/abstract.h>
#include <omen/modules/libraries/definitions/stddef.h>

#ifdef BOOTLOADER_USE_LIMINE
#include <omen/modules/managers/boot/bootloaders/limine/bootservices.h>
#endif

struct bootloader_operations * bootloader_operations;

void init_bootloader() {
    bootloader_operations = get_boot_ops();
}

void (*get_terminal_writer())(const char*, uint64_t) {
    void (*func)(const char*, uint64_t) = (void (*)(const char*, uint64_t))bootloader_operations->get_terminal_writer(NULL);
    return func;
}

char*    get_bootloader_name() {
    char* name = (char*)bootloader_operations->get_bootloader_name(NULL);
    return name;
}

char*    get_bootloader_version() {
    char* version = (char*)bootloader_operations->get_bootloader_version(NULL);
    return version;
}

uint64_t get_terminal_count() {
    uint64_t count = (uint64_t)bootloader_operations->get_terminal_count(NULL);
    return count;
}

uint64_t get_current_terminal() {
    uint64_t terminal = (uint64_t)bootloader_operations->get_current_terminal(NULL);
    return terminal;
}

int64_t  get_boot_time() {
    int64_t time = (int64_t)bootloader_operations->get_boot_time(NULL);
    return time;
}

uint64_t get_memory_map_entries() {
    uint64_t entries = (uint64_t)bootloader_operations->get_memory_map_entries(NULL);
    return entries;
}

uint64_t get_memory_map_base(uint64_t) {
    uint64_t base = (uint64_t)bootloader_operations->get_memory_map_base(NULL);
    return base;
}

uint64_t get_memory_map_length(uint64_t) {
    uint64_t length = (uint64_t)bootloader_operations->get_memory_map_length(NULL);
    return length;
}

uint64_t get_memory_map_type(uint64_t) {
    uint64_t type = (uint64_t)bootloader_operations->get_memory_map_type(NULL);
    return type;
}

uint64_t get_kernel_address_physical() {
    uint64_t address = (uint64_t)bootloader_operations->get_kernel_address_physical(NULL);
    return address;
}

uint64_t get_kernel_address_virtual() {
    uint64_t address = (uint64_t)bootloader_operations->get_kernel_address_virtual(NULL);
    return address;
}

uint64_t get_rsdp_address() {
    uint64_t address = (uint64_t)bootloader_operations->get_rsdp_address(NULL);
    return address;
}

uint64_t get_smbios32_address() {
    uint64_t address = (uint64_t)bootloader_operations->get_smbios32_address(NULL);
    return address;
}

uint64_t get_smbios64_address() {
    uint64_t address = (uint64_t)bootloader_operations->get_smbios64_address(NULL);
    return address;
}

uint32_t get_smp_flags() {
    uint32_t flags = (uint32_t)bootloader_operations->get_smp_flags(NULL);
    return flags;
}

uint32_t get_smp_bsp_lapic_id() {
    uint32_t id = (uint32_t)bootloader_operations->get_smp_bsp_lapic_id(NULL);
    return id;
}

uint64_t get_smp_cpu_count() {
    uint64_t count = (uint64_t)bootloader_operations->get_smp_cpu_count(NULL);
    return count;
}

boot_smp_info_t ** get_smp_cpus() {
    boot_smp_info_t ** cpus = (boot_smp_info_t**)bootloader_operations->get_smp_cpus(NULL);
    return cpus;
}

uint64_t get_framebuffer_count() {
    uint64_t count = (uint64_t)bootloader_operations->get_framebuffer_count(NULL);
    return count;
}

boot_framebuffer_t ** get_framebuffers() {
    boot_framebuffer_t ** framebuffers = (boot_framebuffer_t**)bootloader_operations->get_framebuffers(NULL);
    return framebuffers;
}

void set_terminal_extra_handler() {
    bootloader_operations->set_terminal_extra_handler(NULL);
}

void set_terminal_writer(uint64_t terminal) {
    bootloader_operations->set_terminal_writer((void*)terminal);
}