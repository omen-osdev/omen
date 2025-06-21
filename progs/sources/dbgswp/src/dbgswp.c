#include <stdio.h>
#include <string.h>
#include <errno.h>

void set_debug_level(unsigned long long level) {
    //Syscall 335
    __asm__ volatile("mov $335, %%rax\n"
                         "mov %0, %%rdi\n" // Set debug level
                         "syscall\n"
                         : : "r"(level) : "rax", "rdi");
}

int main(int argc, char* argv[]){
    if(argc < 2){
        fprintf(stderr, "Usage: %s [none|error|warn|strace|info|debug]\n", argv[0]);
        return 1;
    }

    unsigned long long level = 0;
    if (strcmp(argv[1], "none") == 0) {
        level = 0;
    } else if (strcmp(argv[1], "error") == 0) {
        level = 1;
    } else if (strcmp(argv[1], "warn") == 0) {
        level = 2;
    } else if (strcmp(argv[1], "strace") == 0) {
        level = 3; // Assuming strace is equivalent to info
    } else if (strcmp(argv[1], "info") == 0) {
        level = 4;
    } else if (strcmp(argv[1], "debug") == 0) {
        level = 5;
    } else {
        fprintf(stderr, "Invalid debug level: %s\n", argv[1]);
        return 1;
    }

    set_debug_level(level);

    return 0;
}