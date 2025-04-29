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

void test_signals() {

    int original_pid = sys_getpid();
    int pid = sys_fork();
    struct sigaction sa;
    sa.sa_sigaction = NULL;
    sa.sa_flags = 0;
    sa.sa_mask = 0;
    sa.sa_restorer = NULL;

    if (pid == 0) {
        // Child process
        sa.sa_handler = signal_handler_child;
        pid = sys_getppid();
    } else {
        // Parent process
        sa.sa_handler = signal_handler_parent;
    }

    printf("[%d] Sending signal to process %d\n", original_pid, pid);
    sys_sigaction(SIGUSR1, &sa, NULL);
    sys_sched_yield(); //Give them a chance to install the signal handler
    sys_kill(pid, SIGUSR1);
}

int main(int argc, char* argv[], char* envp[]) {
    minilibc_init();
    print_args(argc, argv, envp);

    test_signals();

    while (1) {
        sys_sched_yield();
    }
    return 0;
}