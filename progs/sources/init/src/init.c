#include <minilibc.h>

int main(int argc, char* argv[]) {
    const char text[] = "Hello, World!\n";
    unsigned long long ret = 0;
    unsigned long long arg1 = 1;
    unsigned long long arg2 = (unsigned long long)text;
    unsigned long long arg3 = sizeof(text);
    unsigned long long arg4 = 0;
    unsigned long long arg5 = 0;
    unsigned long long arg6 = 0;
    unsigned long long syscall_number = 1;
    __asm__ volatile ("syscall" : "=a" (ret) : "a" (syscall_number), "D" (arg1), "S" (arg2), "d" (arg3), "r" (arg4), "r" (arg5), "r" (arg6) : "memory");
    while (1) {
    
    }
}