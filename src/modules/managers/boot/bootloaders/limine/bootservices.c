#include <omen/managers/boot/bootloaders/limine/bootservices.h>
#include <omen/managers/boot/bootloaders/limine/limine_requests.h>

#define CALL_SERVICE(x) (x.response)

#define DEFAULT_TERMINAL_WRITER       0x0
uint64_t terminal_writer = DEFAULT_TERMINAL_WRITER;

void _service_error() {
    while (1) {
        __asm__("hlt");
    }
}

void _generic_terminal_callback(struct limine_terminal* terminal, uint64_t type, uint64_t p1, uint64_t p2, uint64_t p3) {
    (void)terminal;
    (void)type;
    (void)p1;
    (void)p2;
    (void)p3;
    //TODO
}

void _set_terminal_extra_handler() {} //TODO

void _generic_terminal_writer(const char* str, uint64_t length) {
    struct limine_terminal *terminal = CALL_SERVICE(TERMINAL)->terminals[terminal_writer];
    terminal_request.response->write(terminal, str, length);
}

void (*_get_terminal_writer())(const char*, uint64_t) {
    return _generic_terminal_writer;
}

char* _get_bootloader_name() {
    return CALL_SERVICE(BOOTLOADER)->name;
}

char* _get_bootloader_version() {
    return CALL_SERVICE(BOOTLOADER)->version;
}

uint64_t _get_terminal_count() {
    return CALL_SERVICE(TERMINAL)->terminal_count;
}

uint64_t _get_current_terminal() {
    return terminal_writer;
}

int64_t _get_boot_time() {
    return CALL_SERVICE(TIME)->boot_time;
}

uint64_t _get_memory_map_entries() {
    return CALL_SERVICE(MEMMAP)->entry_count;
}

uint64_t _get_memory_map_base(uint64_t index) {
    return CALL_SERVICE(MEMMAP)->entries[index]->base;
}
uint64_t _get_memory_map_length(uint64_t index) {
    return CALL_SERVICE(MEMMAP)->entries[index]->length;
}
uint64_t _get_memory_map_type(uint64_t index) {
    return CALL_SERVICE(MEMMAP)->entries[index]->type;
}

uint64_t _get_kernel_address_physical() {
    return CALL_SERVICE(KERNEL)->physical_base;
}

uint64_t _get_kernel_address_virtual() {
    return CALL_SERVICE(KERNEL)->virtual_base;
}

uint64_t _get_rsdp_address() {
    return (uint64_t)CALL_SERVICE(RSDP)->address;
}

uint64_t _get_smbios32_address() {
    return (uint64_t)CALL_SERVICE(SMBIOS)->entry_32;
}

uint64_t _get_smbios64_address() {
    return (uint64_t)CALL_SERVICE(SMBIOS)->entry_64;
}

void _set_terminal_writer(uint64_t terminal) {
    if (terminal >= _get_terminal_count()) {
        _service_error();
    }
    terminal_writer = terminal;
}

uint32_t _get_smp_flags() {
    return CALL_SERVICE(SMP)->flags;
}

uint32_t _get_smp_bsp_lapic_id() {
    return CALL_SERVICE(SMP)->bsp_lapic_id;
}

uint64_t _get_smp_cpu_count() {
    return CALL_SERVICE(SMP)->cpu_count;
}

struct limine_smp_info ** _get_smp_cpus() {
    return (struct limine_smp_info**)(CALL_SERVICE(SMP)->cpus);
}

uint64_t _get_framebuffer_count() {
    return CALL_SERVICE(FRAMEBUFFER)->framebuffer_count;
}

struct limine_framebuffer ** _get_framebuffers() {
    return (struct limine_framebuffer**)(CALL_SERVICE(FRAMEBUFFER)->framebuffers);
}

struct bootloader_operations limine_boot_ops = {
    .get_boot_time = _get_boot_time,
    .get_bootloader_name = _get_bootloader_name,
    .get_bootloader_version = _get_bootloader_version,
    .get_current_terminal = _get_current_terminal,
    .get_framebuffer_count = _get_framebuffer_count,
    .get_framebuffers = (void * (*)(void))_get_framebuffers,
    .get_kernel_address_physical = _get_kernel_address_physical,
    .get_kernel_address_virtual = _get_kernel_address_virtual,
    .get_memory_map_base = _get_memory_map_base,
    .get_memory_map_entries = _get_memory_map_entries,
    .get_memory_map_length = _get_memory_map_length,
    .get_memory_map_type = _get_memory_map_type,
    .get_rsdp_address = _get_rsdp_address,
    .get_smbios32_address = _get_smbios32_address,
    .get_smbios64_address = _get_smbios64_address,
    .get_smp_bsp_lapic_id = _get_smp_bsp_lapic_id,
    .get_smp_cpu_count = _get_smp_cpu_count,
    .get_smp_cpus = (void * (*)(void))_get_smp_cpus,
    .get_smp_flags = _get_smp_flags,
    .get_terminal_count = _get_terminal_count,
    .get_terminal_writer = (void * (*)(void))_get_terminal_writer,
    .set_terminal_extra_handler = _set_terminal_extra_handler,
    .set_terminal_writer = _set_terminal_writer
};

struct bootloader_operations * get_boot_ops() {
    return &limine_boot_ops;
}