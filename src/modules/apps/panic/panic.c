#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>  

__attribute__((noreturn)) void panic(const char * str) {
    kprintf("PANIC: %s\n", str);
    while (1) {}
}