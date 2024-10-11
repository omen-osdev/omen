struct stat {
    int st_dev;
    int st_ino;
    int st_mode;
    int st_nlink;
    int st_uid;
    int st_gid;
    int st_rdev;
    int st_size;
    int st_blksize;
    int st_blocks;
    int st_atime;
    int st_mtime;
    int st_ctime;
};

void sys_read(int fd, char * buffer, int size);
void sys_write(int fd, char * buffer, int size);
int sys_open(const char * path, int flags);
int sys_close(int fd);
int sys_ioctl(int fd, unsigned long request, void * arg);
void sys_sched_yield();
void sys_fork();
void sys_exit(int error_code);
void sys_execve(const char * path, const char * argv, const char * envp);