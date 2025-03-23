#include "minilibc.h"
typedef unsigned long long uint64_t;

uint64_t syscall(uint64_t syscall_number, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    unsigned long long ret;
    __asm__ volatile ("syscall" : "=a" (ret) : "a" (syscall_number), "D" (arg1), "S" (arg2), "d" (arg3), "r" (arg4), "r" (arg5), "r" (arg6) : "memory");
    return ret;
}
void sys_read(int fd, char * buffer, int size) {
    syscall(0, (uint64_t)fd, (uint64_t)buffer, (uint64_t)size, 0, 0, 0);
}
void sys_write(int fd, char * buffer, int size) {
    syscall(1, (uint64_t)fd, (uint64_t)buffer, (uint64_t)size, 0, 0, 0);
}
int sys_open(const char * path, int flags) {
    return syscall(2, (uint64_t)path, (uint64_t)flags, 0, 0, 0, 0);
}
int sys_close(int fd) {
    return syscall(3, (uint64_t)fd, 0, 0, 0, 0, 0);
}
void sys_stat(const char * path, struct stat * stat) {
    syscall(4, (uint64_t)path, (uint64_t)stat, 0, 0, 0, 0);
}
int sys_ioctl(int fd, unsigned long request, void * arg) {
    return syscall(16, (uint64_t)fd, (uint64_t)request, (uint64_t)arg, 0, 0, 0);
}
void sys_sched_yield() {
    syscall(24, 0, 0, 0, 0, 0, 0);
}
short sys_fork() {
    return syscall(57, 0, 0, 0, 0, 0, 0);
}
void sys_execve(const char * path, const char * argv, const char * envp) {
    syscall(59, (uint64_t)path, (uint64_t)argv, (uint64_t)envp, 0, 0, 0);
}
void sys_exit(int error_code) {
    syscall(60, (uint64_t)error_code, 0, 0, 0, 0, 0);
}