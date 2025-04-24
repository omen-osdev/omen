
#include <stdint.h>
#include <signal.h>

#define MAP_SHARED 0x1
#define MAP_PRIVATE 0x2
#define MAP_ANONYMOUS 0x4

#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4
#define PROT_NONE 0x8

#define MS_SYNC 0x0
#define MS_ASYNC 0x1
#define MS_INVALIDATE 0x2

struct stat {
    uint64_t st_dev;
    uint64_t st_ino;
    uint64_t st_mode;
    uint64_t st_nlink;
    uint64_t st_uid;
    uint64_t st_gid;
    uint64_t st_rdev;
    uint64_t st_size;
    uint64_t st_blksize;
    uint64_t st_blocks;
    uint64_t st_atime;
    uint64_t st_mtime;
    uint64_t st_ctime;
};

typedef struct stack {
    void * base;
    int flags;
    void * top;
} stack_t;

typedef struct stat stat_t;

void minilibc_init();
void * get_vdso_signal_trampoline();
void sys_read(int fd, char * buffer, int size);
void sys_write(int fd, char * buffer, int size);
int sys_open(const char * path, int flags);
int sys_close(int fd);
int sys_ioctl(int fd, unsigned long request, void * arg);
int sys_fstat(int fd, struct stat * buf);
int sys_stat(const char * path, struct stat * buf);
int sys_msync(void * addr, size_t length, int flags);
void sys_sched_yield();
short sys_fork();
void sys_exit(int error_code);
void sys_execve(const char * path, const char * argv, const char * envp);
void * sys_mmap(void * addr, size_t length, int prot, int flags, int fd, off_t offset);
void sys_mprotect(void * addr, size_t length, int prot);
void sys_munmap(void * addr, size_t length);
int sys_dup(int fd);
int sys_dup2(int oldfd, int newfd);
int sys_getpid();
int sys_geppid();


int sys_sigaction(int signum, struct sigaction * act, struct sigaction * oldact);
int sys_sigsuspend(sigset_t* sigsuspend_mask, const sigset_t *mask);
int sys_sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int sys_sigpending(sigset_t *set);
int sys_kill(int pid, int sig);
int sys_sigaltstack(const stack_t *ss, stack_t *oss);