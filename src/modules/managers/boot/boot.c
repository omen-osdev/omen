#include <omen/managers/boot/boot.h>
#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/managers/dev/devices.h>
#include <omen/libraries/std/stddef.h>
#include <omen/managers/cpu/process.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/apps/debug/debug.h>
#include <omen/managers/mem/pmm.h>
#include <omen/managers/mem/vmm.h>
#include <omen/managers/cpu/cpu.h>
#include <omen/hal/arch/x86/apic.h>
#include <omen/hal/arch/x86/int.h>
#include <omen/hal/arch/x86/gdt.h>
#include <emulated/dcon.h>
#include <serial/serial.h>
#include <acpi/acpi.h>
#include <omen/managers/dev/fb.h>

void boot_startup() {
    init_bootloader();
    init_simd();
    init_framebuffer();
    clearscreen(0xffffffff);
    init_devices();
    char * dcon = init_dcon_dd();
    if (dcon == NULL) {
        DBG_ERROR("Failed to initialize DCON device\n");
    }

    init_debugger(dcon);
    set_current_tty(dcon);
    pmm_init();
    init_paging();
    init_heap();
    create_gdt();
    init_interrupts();
    init_cpus();
    device_list();
    init_acpi();
    init_serial_dd();
    kprintf("Booting kernel...\n");

    struct madt_header* madt = get_acpi_madt();
    if (madt != 0) {
        register_apic(madt, 0x0);
    }

    kprintf("Booting from %s %s...\n", get_bootloader_name(), get_bootloader_version());
    kprintf("Booting kernel...\n");
    kprintf("Active subsystems: APIC, ACPI, VMM, PMM, HEAP, SERIAL\n");
    kprintf("Enabling interrupts...\n");
    __asm__ volatile("sti");
    kprintf("bad@omen:~/# TODOTODOTODO");
}
