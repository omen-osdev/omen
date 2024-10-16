#include <minilibc/minilibc.h>

void _idle() {
    char text[32] = "I am the ";
    int j = 0;
    while (text[j] != 0) {
        j++;
    }
    
    volatile short pid = sys_fork();

    if (pid == 0) {
        const char child[] = "child\n";
        
        int i = 0;
        while (child[i] != 0) {
            text[j + i] = child[i];
            i++;
        }

        sys_write(1, text, 32);

    } else {
        const char parent[] = "parent\n";

        int i = 0;
        while (parent[i] != 0) {
            text[j + i] = parent[i];
            i++;
        }

        sys_write(1, text, 32);
    }

    while(1) {
        //syscall yield, do not optimize this
        sys_sched_yield();
    }
}