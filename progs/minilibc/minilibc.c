
#include <stdint.h>
#include <minilibc.h>

uint64_t syscall(uint64_t syscall_number, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    unsigned long long ret;
    // The syscall number is passed in rax
    //arg0 is rdi
    //arg1 is rsi
    //arg2 is rdx
    //arg3 is rcx
    //arg4 is r8
    //arg5 is r9
    //return value is in rax

    __asm__ volatile ("mov %0, %%rax\n"
        "mov %1, %%rdi\n"
        "mov %2, %%rsi\n"
        "mov %3, %%rdx\n"
        "mov %4, %%rcx\n"
        "mov %5, %%r8\n"
        "mov %6, %%r9\n"
        "syscall\n" : "=a" (ret) : "r" (syscall_number), "r" (arg1), "r" (arg2), "r" (arg3), "r" (arg4), "r" (arg5), "r" (arg6) : "rcx", "r11", "memory");

    return ret;
}

void sys_read(int fd, char * buffer, int size) {
    syscall(0, (uint64_t)fd, (uint64_t)buffer, (uint64_t)size, 0, 0, 0);
}
void sys_write(int fd, char * buffer, int size) {
    syscall(1, (uint64_t)fd, (uint64_t)buffer, (uint64_t)size, 0, 0, 0);
}
int sys_open(const char * path, int flags) {
    (void)path;
    return syscall(2, (uint64_t)path, (uint64_t)flags, 0, 0, 0, 0);
}
int sys_close(int fd) {
    return syscall(3, (uint64_t)fd, 0, 0, 0, 0, 0);
}
int sys_ioctl(int fd, unsigned long request, void * arg) {
    return syscall(16, (uint64_t)fd, (uint64_t)request, (uint64_t)arg, 0, 0, 0);
}
int sys_fstat(int fd, struct stat * buf) {
    return syscall(5, (uint64_t)fd, (uint64_t)buf, 0, 0, 0, 0);
}
int sys_stat(const char * path, struct stat * buf) {
    return syscall(4, (uint64_t)path, (uint64_t)buf, 0, 0, 0, 0);
}
void sys_sched_yield() {
    syscall(24, 0, 0, 0, 0, 0, 0);
}
short sys_fork() {
    return syscall(57, 0, 0, 0, 0, 0, 0);
}
void sys_exit(int error_code) {
    syscall(60, (uint64_t)error_code, 0, 0, 0, 0, 0);
}
void sys_execve(const char * path, const char * argv, const char * envp) {
    syscall(59, (uint64_t)path, (uint64_t)argv, (uint64_t)envp, 0, 0, 0);
}
void * sys_mmap(void * addr, size_t length, int prot, int flags, int fd, off_t offset) {
    return (void *)syscall(9, (uint64_t)addr, (uint64_t)length, (uint64_t)prot, (uint64_t)flags, (uint64_t)fd, (uint64_t)offset);
}
void sys_mprotect(void * addr, size_t length, int prot) {
    syscall(10, (uint64_t)addr, (uint64_t)length, (uint64_t)prot, 0, 0, 0);
}
void sys_munmap(void * addr, size_t length) {
    syscall(11, (uint64_t)addr, (uint64_t)length, 0, 0, 0, 0);
}