#ifndef _LIMINE_REQUESTS_H
#define _LIMINE_REQUESTS_H

#define BOOTLOADER  bootloader_info_request
#define TERMINAL    terminal_request
#define MEMMAP      memmap_request
#define KERNEL      kernel_address_request
#define HHDM        hhdm_request
#define RSDP        rsdp_request
#define SMBIOS      smbios_request
#define TIME        time_request
#define SMP         smp_request
#define FRAMEBUFFER framebuffer_request

static volatile struct limine_boot_time_request time_request = {
    .id = LIMINE_BOOT_TIME_REQUEST,
    .revision = 0
};
static volatile struct limine_bootloader_info_request bootloader_info_request = {
    .id = LIMINE_BOOTLOADER_INFO_REQUEST,
    .revision = 0
};
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};
static volatile struct limine_kernel_address_request kernel_address_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};
static volatile struct limine_smp_request smp_request = {
    .id = LIMINE_SMP_REQUEST,
    .flags = 1,
    .revision = 0
};
static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};
static volatile struct limine_smbios_request smbios_request = {
    .id = LIMINE_SMBIOS_REQUEST,
    .revision = 0
};
static volatile struct limine_stack_size_request stack_size_request = {
    .id = LIMINE_STACK_SIZE_REQUEST,
    .revision = 0,
    .stack_size = 0x2000000
};
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 1
};
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};
#endif // _LIMINE_REQUESTS_H
