#include <omen/managers/boot/boot.h>
#include "./drivers/emulated/serial.h"

__attribute__((noreturn)) void _halt() {
    while (1) {
        __asm__("hlt");
    }
}

__attribute__((noreturn)) void _start() {
    boot_startup();
    _halt();
}
