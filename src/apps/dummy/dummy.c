#include "dummy.h"
#include "minilibc.h"

int dummy_main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    short pid = sys_fork();
    if (pid == 0) {
        while (1) {
            sys_write(1, "b", 1);
            sys_sched_yield();
        }
    } else {
        while (1) {
            sys_write(1, "a", 1);
            sys_sched_yield();
        }
    }
}