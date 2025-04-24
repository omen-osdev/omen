#include <minilibc.h>
#include <stdio.h>
#include <string.h>
#include <cpuid.h>
#include <auxv.h>
#include <vdso.h>

void signal_handler(int signum) {
    printf("Signal %d received\n", signum);
}

int main(int argc, char* argv[], char* envp[]) {
    minilibc_init();
    printf("Init starting");
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

    int pid = sys_getpid();
    printf("Process ID: %d\n", pid);

    void * trampoline = get_vdso_signal_trampoline();
    printf("Signal trampoline: %p\n", trampoline);

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_sigaction = NULL;
    sa.sa_flags = 0;
    sa.sa_mask = 0;
    sa.sa_restorer = NULL;
    
    sys_sigaction(SIGUSR1, &sa, NULL);
 
    //Send signal to self
    sys_kill(pid, SIGUSR1);
    printf("Signal sent to self\n");

    while (1) {
        sys_sched_yield();
    }
    return 0;
}