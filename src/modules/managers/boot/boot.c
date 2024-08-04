#include <omen/managers/boot/boot.h>
#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/managers/dev/devices.h>
#include <omen/libraries/std/stddef.h>
#include <omen/managers/cpu/process.h>
#include <emulated/dcon.h>

void boot_startup() {
    init_bootloader();
    init_devices();

    init_dcon_dd();
    set_current_tty("/dev/dcoma");
    device_list();

    kprintf("Booting from %s %s\n", get_bootloader_name(), get_bootloader_version());

    while (1) {
        __asm__("hlt");
    }
}