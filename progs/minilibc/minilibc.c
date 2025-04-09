
#include <stdint.h>
#include <minilibc.h>

static inline int64_t syscall(int64_t function, int64_t arg1, int64_t arg2, int64_t arg3, int64_t arg4, int64_t arg5, int64_t arg6)
{
    int64_t result;

    register int64_t r10 __asm__("r10") = arg4;
    register int64_t r8 __asm__("r8") = arg5;
    register int64_t r9 __asm__("r9") = arg6;

    __asm__ volatile (
        "syscall"
        : "=a"(result)
        : "a"(function),
          "D"(arg1),
          "S"(arg2),
          "d"(arg3),
          "r"(r10),
          "r"(r8),
          "r"(r9)
        : "memory"
    );

    return result;
}

void sys_read(int fd, char * buffer, int size) {
    syscall(0, (int64_t)fd, (int64_t)buffer, (int64_t)size, 0, 0, 0);
}
void sys_write(int fd, char * buffer, int size) {
    syscall(1, (int64_t)fd, (int64_t)buffer, (int64_t)size, 0, 0, 0);
}
int sys_open(const char * path, int flags) {
    return (int)syscall(2, (int64_t)path, (int64_t)flags, 0, 0, 0, 0);
}
int sys_close(int fd) {
    return (int)syscall(3, (int64_t)fd, 0, 0, 0, 0, 0);
}
int sys_ioctl(int fd, unsigned long request, void * arg) {
    return (int)syscall(16, (int64_t)fd, (int64_t)request, (int64_t)arg, 0, 0, 0);
}
int sys_fstat(int fd, struct stat * buf) {
    return (int)syscall(5, (int64_t)fd, (int64_t)buf, 0, 0, 0, 0);
}
int sys_stat(const char * path, struct stat * buf) {
    return (int)syscall(4, (int64_t)path, (int64_t)buf, 0, 0, 0, 0);
}
void sys_sched_yield() {
    syscall(24, 0, 0, 0, 0, 0, 0);
}
short sys_fork() {
    return (short)syscall(57, 0, 0, 0, 0, 0, 0);
}
void sys_exit(int error_code) {
    syscall(60, (int64_t)error_code, 0, 0, 0, 0, 0);
}
void sys_execve(const char * path, const char * argv, const char * envp) {
    syscall(59, (int64_t)path, (int64_t)argv, (int64_t)envp, 0, 0, 0);
}
void * sys_mmap(void * addr, size_t length, int prot, int flags, int fd, off_t offset) {
    return (void *)syscall(9, (int64_t)addr, (int64_t)length, (int64_t)prot, (int64_t)flags, (int64_t)fd, (int64_t)offset);
}
void sys_mprotect(void * addr, size_t length, int prot) {
    syscall(10, (int64_t)addr, (int64_t)length, (int64_t)prot, 0, 0, 0);
}
void sys_munmap(void * addr, size_t length) {
    syscall(11, (int64_t)addr, (int64_t)length, 0, 0, 0, 0);
}