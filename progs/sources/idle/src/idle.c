#include <minilibc.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    (void)argc;
    minilibc_init();
    int pid = sys_fork();
    if (pid == 0) {
        char ** cenvp = NULL;
        char ** cargv = malloc(sizeof(char*) * 2);
        if (cargv == NULL) {
            sys_exit(1);
        }
        cargv[0] = malloc(strlen(argv[1]) + 1);
        if (cargv[0] == NULL) {
            sys_exit(1);
        }
        cargv[1] = NULL;
        strcpy(cargv[0], argv[1]);
        cenvp = malloc(sizeof(char*) * 2);
        if (cenvp == NULL) {
            sys_exit(1);
        }
        cenvp[0] = NULL;
        cenvp[1] = NULL;
        sys_execve(argv[1], cargv, cenvp);
        sys_exit(1);
    } else {
        while(1) {
            sys_sched_yield();
        }
    }
    return 0;
}