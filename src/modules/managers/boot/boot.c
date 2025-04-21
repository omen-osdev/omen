#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/libraries/executables/loader.h>
#include <omen/libraries/crypto/md5.h>
#include <omen/libraries/std/stddef.h>
#include <omen/libraries/std/string.h>

#include <omen/hal/arch/x86/getcpuid.h>
#include <omen/hal/arch/x86/apic.h>
#include <omen/hal/arch/x86/int.h>
#include <omen/hal/arch/x86/gdt.h>
#include <omen/hal/arch/x86/vm.h>

#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/managers/dev/devices.h>
#include <omen/managers/cpu/process.h>
#include <omen/managers/boot/boot.h>
#include <omen/managers/mem/pmm.h>
#include <omen/managers/mem/vmm.h>
#include <omen/managers/cpu/cpu.h>
#include <omen/managers/dev/fb.h>
#include <omen/managers/dev/pit.h>

#include <omen/apps/debug/dshell.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>

#include <emulated/dcon.h>
#include <serial/serial.h>
#include <serial/serial_dd.h>
#include <serial/serial_interface.h>
#include <acpi/acpi.h>
#include <ps2/ps2.h>
#include <pci/pci.h>
#include <disk/disk.h>
#include <fifo/fifo.h>
#include <tty/tty.h>
#include <tty/tty_dd.h>
#include <tty/tty_interface.h>

#include <vfs/vfs.h>
#include <vfs/vfs_interface.h>
#include <vfs/generic/fifo/generic_fifo.h>
#include <vfs/generic/ext2/generic_ext2.h>
#include <vfs/generic/tty/generic_tty.h>

#include <fifo/fifo_interface.h>

char * enable_tty_over_serial(uint8_t reserved) {
    init_serial(4096, 4096);
    int port_count = serial_count_ports();
    int *port_buffer = kmalloc(sizeof(int) * port_count);
    serial_get_ports(port_buffer);

    for (int i = 0; i < port_count; i++) {
        char* name = device_create((void*)0x0, DEVICE_SERIAL, port_buffer[i]);
        int index = tty_init(name, name, TTY_MODE_SERIAL, 1024, 1024);
        struct tty * tty = get_tty(index);
        if (is_valid_tty(tty)) {
            kprintf("TTY with index: %d and name %s\n", index, device_create((void*)tty, DEVICE_TTY, index));
            serial_write_now(name, "serial> ", 0, 8);
        }
    }

    kfree(port_buffer);
    device_list();

    const char prompt[] = "tty> ";
    uint32_t terminals = get_device_count_by_major(DEVICE_TTY);
    if (terminals > reserved) {
        uint8_t index = 0;
        struct device* current = get_first_device();

        while (current != 0) {
            if ((current->bc == 0) && (current->major == DEVICE_TTY)) {
                if (index++ >= reserved) {
                    kprintf("Enabling debugger on %s\n", current->name);
                    tty_write_now(current->name, prompt, 0, strlen(prompt));
                    return current->name;
                }
            }
            current = get_next_device(current);
        }
    }

    return NULL;
}

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
    kprintf("Early startup complete...\n");

    if (get_maxphyaddr() != MAXPHYADDR) {
        panic("Invalid MAXPHYADDR\n");
    }

    pmm_init();
    init_paging();
    create_gdt();
    init_pit(50);
    init_interrupts();
    init_cpus();
    init_acpi();
    struct madt_header* madt = get_acpi_madt();
    if (madt != 0) {
        register_apic(madt, 0x0);
    }
    kprintf("Secondary startup complete... starting drivers and devices\n");
    init_drive();
    init_fifo_dd();
    init_serial_dd();
    init_tty_dd();
    init_ps2_dd(fb_get_width(), fb_get_height());
    init_pci();

    kprintf("Create dummy fifo device\n");
    char * fifo = create_fifo(1024);
    if (fifo == NULL) {
        panic("Failed to create FIFO device\n");
    }
    kprintf("FIFO device created: %s\n", fifo);
    kprintf("Detecting main tty over serial...\n");
    char * tty = enable_tty_over_serial(0);
    if (tty == NULL) {
        panic("Failed to enable debugger\n");
    }
    init_debugger(tty);
    kprintf("Device startup complete...\n");
    device_list();
    kprintf("Starting VFS...\n");
    register_filesystem(fifo_registrar);
    register_filesystem(ext2_registrar);
    register_filesystem(tty_registrar);
    probe_fs();
    kprintf("VFS startup complete...\n");
    vfs_lsdisk();
    char vfs_tty[32];
    sprintf(vfs_tty, "%sp0/", tty); 

    //Careful, somehow this shit crashes when you rewrite an string 
    //Map ffffffff80000000 - ffffffff803a000 is read only! diagnose this

    kprintf("Booting from %s %s...\n", get_bootloader_name(), get_bootloader_version());
    kprintf("Booting kernel...\n");
    kprintf("Active subsystems: APIC, ACPI, VMM, PMM, HEAP, SERIAL, TTY, FIFO, EXT2, VFS\n");
    kprintf("Enabling interrupts...\n");
    mask_interrupt(PIT_IRQ);
    __asm__ volatile("sti");

    kprintf("Entering uspace...\n");
    init_process("hdap2/export/init.elf", "hdap2/export/idle.elf", vfs_tty);
    
    panic("¡Returned from the scheduler!\n");
}
