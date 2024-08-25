#include <omen/managers/boot/boot.h>
#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/managers/dev/devices.h>
#include <omen/libraries/std/stddef.h>
#include <omen/managers/cpu/process.h>
#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/managers/mem/pmm.h>
#include <omen/managers/cpu/cpu.h>
#include <emulated/dcon.h>
#include <serial/serial.h>

void boot_startup() {
    init_bootloader();
    init_devices();
    char * dcon = init_dcon_dd();
    if (dcon == NULL) {
        DBG_ERROR("Failed to initialize DCON device\n");
    }

    init_debugger(dcon);
    set_current_tty(dcon);
    init_cpus();
    device_list();


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

    pmm_init();
    pmm_list_map();

    char * buffer = pmm_alloc(4024);
    if (buffer == NULL) {
        kprintf("Failed to allocate buffer, jonbardo plz fix\n");
        return;
    }

    kprintf("Allocated buffer at 0x%x\n", buffer);
    strcpy(buffer, "Hello, world!\n");
    strcat(buffer, "This is a test buffer\n");
    kprintf("Buffer contents: %s\n", buffer);
    pmm_free(buffer);

    kprintf("Booting kernel\n");
}
