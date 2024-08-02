#include <omen/managers/boot/boot.h>

__attribute__((noreturn)) void _start() {
    boot_startup();
    while (1);
}