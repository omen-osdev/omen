#include <minilibc.h>
#include <stdio.h>
#include <string.h>
#include <cpuid.h>
#include <auxv.h>
#include <vdso.h>

void signal_handler_parent(int signum) {
    printf("[parent]Signal %d received\n", signum);
}

void signal_handler_child(int signum) {
    printf("[child]Signal %d received\n", signum);
}

void print_args(int argc, char* argv[], char* envp[]) {
    printf("Argc: %d", argc);
    for (int i = 0; i < argc; i++) {
        printf("Argv[%d]: %s", i, argv[i]);
    }

    int envc = 0;
    if (envp != 0)
        for (envc = 0; envp[envc] != NULL; envc++);

    for (int i = 0; i < envc; i++) {
        printf("Envp[%d]: %s", i, envp[i]);
    }
    printf("\n");
}

void test_sleep() {
    int pid = sys_fork();
    if (pid == 0) {
        while (1) {
            struct timespec req = {7, 0};
            sys_nanosleep(&req, NULL);
            printf("[child]Woke up from sleep\n");
        }
    } else {
        while (1) {
            struct timespec req = {3, 0};
            sys_nanosleep(&req, NULL);
            printf("[parent]Woke up from sleep\n");
        }
    }
}

int main(int argc, char* argv[], char* envp[]) {
    minilibc_init();
    print_args(argc, argv, envp);

    int pid = sys_fork();
    if (pid == 0) {
        test_sleep();
    } else {
        while (1) {
            sys_sched_yield();
        }
    }
}