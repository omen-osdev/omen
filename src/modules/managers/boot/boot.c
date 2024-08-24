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
