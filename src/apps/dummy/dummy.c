#include "dummy.h"
#include <omen/apps/debug/debug.h>

int dummy_main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    //call syscall 0 passing it value 0x69 and print the return value
    int ret;
    __asm__ volatile ("syscall" : "=a" (ret) : "a" (0), "D" (0x69) : "memory");

    kprintf("Returned from syscall 0 with value %d\n", ret);

    while (1) {

    }
}