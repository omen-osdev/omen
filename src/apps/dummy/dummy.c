#include "dummy.h"
#include "minilibc.h"

int dummy_main(int argc, char **argv) {
    (void)argc;
    (void)argv;


    while (1) {
        sys_write(1, "a", 1);
        sys_sched_yield();
    }
}