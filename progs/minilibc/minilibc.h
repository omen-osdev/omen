
#include <stdint.h>

#define MAP_SHARED 0x1
#define MAP_PRIVATE 0x2
#define MAP_ANONYMOUS 0x4

#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4
#define PROT_NONE 0x8

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

typedef struct stat stat_t;

void sys_read(int fd, char * buffer, int size);
void sys_write(int fd, char * buffer, int size);
int sys_open(const char * path, int flags);
int sys_close(int fd);
int sys_ioctl(int fd, unsigned long request, void * arg);
int sys_fstat(int fd, struct stat * buf);
int sys_stat(const char * path, struct stat * buf);
void sys_sched_yield();
short sys_fork();
void sys_exit(int error_code);
void sys_execve(const char * path, const char * argv, const char * envp);
void * sys_mmap(void * addr, size_t length, int prot, int flags, int fd, off_t offset);
void sys_mprotect(void * addr, size_t length, int prot);
void sys_munmap(void * addr, size_t length);