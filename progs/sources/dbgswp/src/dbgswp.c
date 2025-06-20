#include <stdio.h>
#include <string.h>
#include <errno.h>

void disable_debugger() {
    //Syscall 335
    __asm__ volatile("mov $335, %%rax\n"
                         "mov $0, %%rdi\n" // 0 to disable debugger
                         "syscall\n"
                         : : : "rax", "rdi");
}

void enable_debugger() {
    //Syscall 335
    __asm__ volatile("mov $335, %%rax\n"
                         "mov $1, %%rdi\n" // 1 to enable debugger
                         "syscall\n"
                         : : : "rax", "rdi");
}

int main(int argc, char* argv[]){
    if(argc < 2){
        fprintf(stderr, "Usage: %s [on|off]\n", argv[0]);
        return 1;
    }

    if(strcmp(argv[1], "on") == 0){
        enable_debugger();
        printf("Debugger enabled\n");
    } else if(strcmp(argv[1], "off") == 0){
        disable_debugger();
        printf("Debugger disabled\n");
    } else {
        fprintf(stderr, "Invalid argument: %s. Use 'on' or 'off'.\n", argv[1]);
        return 1;
    }

    return 0;
}