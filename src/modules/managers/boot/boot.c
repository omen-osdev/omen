#include <omen/managers/boot/boot.h>
#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/managers/dev/devices.h>
#include <omen/libraries/std/stddef.h>
#include <omen/managers/cpu/process.h>
#include <omen/libraries/std/string.h>
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

void boot_startup() {
    init_bootloader();
    init_devices();
    char * dcon = init_dcon_dd();
    if (dcon == NULL) {
        DBG_ERROR("Failed to initialize DCON device\n");
    }

    init_debugger(dcon);
    set_current_tty(dcon);
    pmm_init();
    init_paging();
    create_gdt();
    init_interrupts();
    init_cpus();
    device_list();
    init_acpi();
    
    struct madt_header* madt = get_acpi_madt();
    if (madt != 0) {
        register_apic(madt, 0x0);
    }

    //Initialize the serial port
    if(init_serial_dd() != SUCCESS)
    {
        DBG_ERROR("Failed to initialize Serial driver\n");
    } else {
        DBG_INFO("Serial driver was initialized successfully!\n");
        char * main_serial = create_serial_dd(COM1_BASEADDR, baud_115200);
        if (main_serial == NULL) {
            DBG_ERROR("Failed to create serial device\n");
        } else {
            DBG_INFO("Serial device %s created successfully\n", main_serial);
        }
    }

    kprintf("Booting from %s %s\n", get_bootloader_name(), get_bootloader_version());

    kprintf("Booting kernel...\n");
    __asm__ volatile("sti");
    kprintf("Triggering a page fault\n");
    uint64_t* ptr = (uint64_t*)0x0;
    *ptr = 0x0;

}
