#include <minilibc.h>
#include <stdio.h>
#include <string.h>
#include <cpuid.h>
#include <auxv.h>
#include <vdso.h>
#include <wait.h>

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

int main(int argc, char* argv[], char* envp[]) {
    minilibc_init();
    //print_args(argc, argv, envp);
    struct timespec ts, res;
    sys_clock_gettime(CLOCK_MONOTONIC, &ts);
    printf("Current time: %ld.%09ld\n", ts.tv_sec, ts.tv_nsec);
    sys_clock_getres(CLOCK_MONOTONIC, &res);
    printf("Clock resolution: %ld.%09ld\n", res.tv_sec, res.tv_nsec);

    //Example of fork and waitpid, spawn a child process and wait for it to finish
    int pid = sys_fork();
    if (pid == 0) {
        // Child process
        printf("[CHILD]Child process PID: %d\n", sys_getpid());
        sys_exit(5);
    } else {
        printf("[PARENT]Child process PID: %d\n", pid);
        int status = 0;
        int wait_pid = sys_waitpid(pid, &status, 0);
        if (wait_pid == pid) {
            printf("Child process %d exited with status %d\n", pid, WEXITSTATUS(status));
        } else {
            printf("Error waiting for child process\n");
        }
    }

    while (1) {
        // Infinite loop to keep the process alive
    }
}