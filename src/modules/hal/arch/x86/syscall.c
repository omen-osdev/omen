#include <omen/hal/arch/x86/syscall.h>
#include <omen/hal/arch/x86/msr.h>
#include <omen/hal/arch/x86/cpu.h>
#include <omen/managers/cpu/process.h>
#include <omen/managers/cpu/vmarea.h>
#include <omen/managers/cpu/signal.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/std/time.h>
#include <omen/libraries/crypto/md5.h>
#include <omen/apps/debug/debug.h>
#include <omen/managers/cpu/sline.h>
#include <omen/apps/panic/panic.h>
#include <vfs/vfs.h>
#include <vfs/vfs_interface.h>
#include <errno.h>
#include <asm/prctl.h>
#include <omen/libraries/std/select.h>
#include <omen/libraries/std/statx.h>
/*


log 
thread_exit
clock_get
sleep
sigrestore ?¿
waitpid

dir_read_entries
dir_remove
dir_create
unlink_at
path_stat
fd_stat
getcwd
chdir

ppoll
pselect

kot's

#define SYS_LOG                 0 no, unnecessary
#define SYS_ARCH_PRCTL          1 ok
#define SYS_GET_TID             2 ok
#define SYS_FUTEX_WAIT          3 not implemented
#define SYS_FUTEX_WAKE          4 not implemented

#define SYS_MMAP                5 ok
#define SYS_MUNMAP              6 ok
#define SYS_MPROTECT            7 ok

#define SYS_EXIT                8 ok
#define SYS_THREAD_EXIT         9 ok
#define SYS_CLOCK_GET           10 ok
#define SYS_CLOCK_GETRES        11 ok
#define SYS_SLEEP               12 ok

#define SYS_SIGPROCMASK         13 ok
#define SYS_SIGACTION           14 ok
#define SYS_SIGRESTORE          15 no, we use sigreturn
#define SYS_FORK                16 ok
#define SYS_WAITPID             17 ok
#define SYS_EXECVE              18 ok
#define SYS_GETPID              19 ok
#define SYS_GETPPID             20 ok
#define SYS_KILL                21 ok

#define SYS_FILE_OPEN           22 ok
#define SYS_FILE_READ           23 ok
#define SYS_FILE_WRITE          24 ok
#define SYS_FILE_SEEK           25 ok
#define SYS_FILE_CLOSE          26 ok
#define SYS_FILE_IOCTL          27 ok

#define SYS_DIR_READ_ENTRIES    28
#define SYS_DIR_REMOVE          29
#define SYS_DIR_CREATE          30
#define SYS_UNLINK_AT           31
#define SYS_RENAME_AT           32
#define SYS_PATH_STAT           33
#define SYS_FD_STAT             34 ok
#define SYS_FCNTL               35
#define SYS_GETCWD              36
#define SYS_CHDIR               37

#define SYS_SOCKET              38 no
#define SYS_BIND                39 no
#define SYS_CONNECT             40 no
#define SYS_LISTEN              41 no
#define SYS_ACCEPT              42 no
#define SYS_SOCKET_SEND         43 no
#define SYS_SOCKET_RECV         44 no
#define SYS_SOCKET_PAIR         45 no
#define SYS_PPOLL               46 no
#define SYS_SELECT              47 no

*/

extern void setFsBase(uint64_t base);

#define SYSRET(ctx, val) ctx->rax = (uint64_t)val; return;
#define SYSCALL_ARG0(ctx) ctx->rdi
#define SYSCALL_ARG1(ctx) ctx->rsi
#define SYSCALL_ARG2(ctx) ctx->rdx
#define SYSCALL_ARG3(ctx) ctx->r10
#define SYSCALL_ARG4(ctx) ctx->r8
#define SYSCALL_ARG5(ctx) ctx->r9
extern void syscall_entry();

int64_t dummy_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    DBG_STRACE("[PID: %d | TID %d] DUMMY_SYSCALL(%d)\n", thread->process->pid, thread->id, ctx->rax);
    return SYSCALL_SUCCESS;
}

int64_t futex_wait_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    DBG_STRACE("[PID: %d | TID %d] FUTEX_WAIT()\n", thread->process->pid, thread->id);
    return SYSCALL_SUCCESS;
}

int64_t futex_wake_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    DBG_STRACE("[PID: %d | TID %d] FUTEX_WAKE()\n", thread->process->pid, thread->id);
    return SYSCALL_SUCCESS;
}

int64_t pselect_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    int nfds = SYSCALL_ARG0(ctx);
    fd_set * readfds = (fd_set *)SYSCALL_ARG1(ctx);
    fd_set * writefds = (fd_set *)SYSCALL_ARG2(ctx);
    fd_set * exceptfds = (fd_set *)SYSCALL_ARG3(ctx);
    struct timespec * timeout = (struct timespec *)SYSCALL_ARG4(ctx);
    int * num_events = (int *)SYSCALL_ARG5(ctx);
    DBG_STRACE("[PID: %d | TID %d] PSELECT_SYSCALL(%d, %p, %p, %p, %p, %p)\n", 
           thread->process->pid, thread->id, nfds, readfds, writefds, exceptfds, timeout, num_events);
    
    struct timespec current_time;
    timespec_now(&current_time);

    struct timespec end_time;
    if (timeout) {
        end_time.tv_sec = current_time.tv_sec + timeout->tv_sec;
        end_time.tv_nsec = current_time.tv_nsec + timeout->tv_nsec;
        if (end_time.tv_nsec >= 1000000000) {
            end_time.tv_sec += 1;
            end_time.tv_nsec -= 1000000000;
        }
    } else {
        end_time = (struct timespec){ .tv_sec = 0, .tv_nsec = 0 };
    }

    uint8_t read_mask[128];
    uint8_t write_mask[128];
    uint8_t except_mask[128];

    if(readfds != NULL){
        memcpy(read_mask, readfds, sizeof(fd_set));
        memset(readfds, 0, sizeof(fd_set));
    }else{
        memset(read_mask, 0, sizeof(fd_set));
    }

    if(writefds != NULL){
        memcpy(write_mask, writefds, sizeof(fd_set));
        memset(writefds, 0, sizeof(fd_set));
    }else{
        memset(write_mask, 0, sizeof(fd_set));
    }

    if(exceptfds != NULL){
        memcpy(except_mask, exceptfds, sizeof(fd_set));
        memset(exceptfds, 0, sizeof(fd_set));
    }else{
        memset(except_mask, 0, sizeof(fd_set));
    }

    int event_count = 0;
    
    do {
        if (event_count > 0) {
            // If we already have events, break out of the loop
            break;
        }

        event_count = 0;

        for(int i = 0; i < 128 && i * 8 < nfds; i++){
            for(int j = 0; j < 8 && (i * 8 + j) < nfds; j++){
                int fd = i * 8 + j;
                int pfd = get_open_file(thread->process, fd);
                if (pfd < 0) {
                    DBG_ERROR("Invalid file descriptor %d\n", fd);
                    continue;
                }

                int events = 0;
                int revents = 0;

                if((read_mask[i] >> j) & 0b1){
                    events |= VFS_POLLIN;
                }

                if((write_mask[i] >> j) & 0b1){
                    events |= VFS_POLLOUT;
                }

                if((except_mask[i] >> j) & 0b1){
                    // TODO
                }

                int evs = vfs_file_event(pfd, events, &revents);
                if (evs < 0) {
                    DBG_ERROR("Error checking events for fd %d\n", fd);
                    continue;
                }

                if (evs > 0) {
                    event_count += evs;
                }

                if (revents & VFS_POLLIN) {
                    if (readfds != NULL) {
                        readfds->fds_bits[fd / 8] |= (1 << (fd % 8));
                    }
                }

                if (revents & VFS_POLLOUT) {
                    if (writefds != NULL) {
                        writefds->fds_bits[fd / 8] |= (1 << (fd % 8));
                    }
                }
            }
        }

        if (end_time.tv_sec != 0 || end_time.tv_nsec == 0) {
            // Check if we reached the timeout
            struct timespec now;
            timespec_now(&now);
            if (now.tv_sec > end_time.tv_sec || 
                (now.tv_sec == end_time.tv_sec && now.tv_nsec >= end_time.tv_nsec)) {
                DBG_WARN("[PID: %d | TID %d] PSELECT_SYSCALL timed out\n", thread->process->pid, thread->id);
                break;
            }
        }
    } while (1);

    *num_events = event_count;

    return SYSCALL_SUCCESS;
}

int64_t read_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    uint64_t fd = SYSCALL_ARG0(ctx);
    uint64_t buffer = SYSCALL_ARG1(ctx);
    uint64_t size = SYSCALL_ARG2(ctx);
    DBG_STRACE("[PID: %d | TID %d] READ_SYSCALL(%d,%d,%d)\n", thread->process->pid, thread->id, fd, buffer, size);
    char * rbuffer = (char*)kmalloc(size + 1024);
    if (!rbuffer) {
        DBG_ERROR("Could not allocate buffer for read\n");
        return SYSCALL_ERROR;
    }

    int pfd = get_open_file(thread->process, fd);
    if (pfd < 0) {
        DBG_ERROR("Invalid file descriptor %d\n", fd);
        kfree(rbuffer);
        return SYSCALL_ERROR;
    }

    memset(rbuffer, 0, size + 1024);
    int64_t res = vfs_file_read(pfd, (void*)rbuffer, size);
    //Print 10 bytes
    DBG_DEBUG("Read %d bytes\n", res);
    if (res < 0) {
        return SYSCALL_ERROR;
    }
    if ((uint64_t)res < size) {
        memcpy((void*)buffer, rbuffer, res);
        return res;
    } else {
        memcpy((void*)buffer, rbuffer, size);
        return size;
    }
}

int64_t write_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    uint64_t fd = SYSCALL_ARG0(ctx);
    uint64_t buffer = SYSCALL_ARG1(ctx);
    uint64_t size = SYSCALL_ARG2(ctx);
    (void)fd;
    (void)buffer;
    (void)size;

    if (size != 1)
        DBG_STRACE("[PID: %d | TID %d] WRITE_SYSCALL(%d,%d,%d)\n", thread->process->pid, thread->id, fd, buffer, size);

    int pfd = get_open_file(thread->process, fd);
    if (pfd < 0) {
        DBG_ERROR("Invalid file descriptor %d\n", fd);
        return SYSCALL_ERROR;
    }

    int64_t res = vfs_file_write(pfd, (void*)buffer, size);
    if (res < 0) {
        DBG_ERROR("Error writing to file descriptor %d\n", fd);
        return SYSCALL_ERROR;
    }
    vfs_file_flush(pfd);

    return res;
}

int64_t dir_open_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    char * path = SYSCALL_ARG0(ctx);
    DBG_STRACE("[PID: %d | TID %d] DIR_OPEN_SYSCALL(%s)\n", thread->process->pid, thread->id, path);
    
    int fd = vfs_dir_open(thread->process->fs, path);
    if (fd < 0) {
        return SYSCALL_ERROR;
    }
    vfs_dir_load(fd);
    return add_open_file(thread->process, fd);
}

int64_t open_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    char* path = SYSCALL_ARG0(ctx);
    int flags = SYSCALL_ARG1(ctx);
    int mode = SYSCALL_ARG2(ctx);
    DBG_STRACE("[PID: %d | TID %d] OPEN_SYSCALL(%s,%d,%d)\n", thread->process->pid, thread->id, path, flags, mode);
    
    if (thread->process->open_files_count >= MAX_OPEN_FILES) {
        DBG_ERROR("Max open files reached\n");
        return SYSCALL_ERROR;
    }
    
    int fd = vfs_file_open(thread->process->fs, path, flags, mode);
    if (fd < 0) {
        return SYSCALL_ERROR;
    }

    return add_open_file(thread->process, fd);
}

int64_t getcwd_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    char * buffer = (char *)SYSCALL_ARG0(ctx);
    size_t size = SYSCALL_ARG1(ctx);
    DBG_STRACE("[PID: %d | TID %d] GETCWD_SYSCALL(%s,%d)\n", thread->process->pid, thread->id, buffer, size);
    
    if (buffer == NULL || size == 0) {
        return SYSCALL_ERROR;
    }
    
    char * cwd = getcwd(thread->process);
    if (cwd == NULL) {
        DBG_ERROR("Could not get cwd\n");
        return SYSCALL_ERROR;
    }

    if (strlen(cwd) > size) {
        DBG_ERROR("Buffer too small\n");
        return SYSCALL_ERROR;
    } else {
        memcpy(buffer, cwd, strlen(cwd));
    }

    return SYSCALL_SUCCESS;
}

int64_t chdir_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    char * path = (char *)SYSCALL_ARG0(ctx);
    DBG_STRACE("[PID: %d | TID %d] CHDIR_SYSCALL(%s)\n", thread->process->pid, thread->id, path);
    
    if (path == NULL) {
        return SYSCALL_ERROR;
    }
    
    char *cwd = getcwd(thread->process);
    if (cwd == NULL) {
        DBG_ERROR("Could not get cwd\n");
        return SYSCALL_ERROR;
    }
    DBG_DEBUG("Current working directory before chdir: %s\n", cwd);
    chdir(thread->process, path);
    cwd = getcwd(thread->process);
    if (cwd == NULL) {
        DBG_ERROR("Could not get cwd after chdir\n");
        return SYSCALL_ERROR;
    }
    DBG_DEBUG("Current working directory after chdir: %s\n", cwd);
    return SYSCALL_SUCCESS;
}

int64_t rmdir_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    char * path = (char *)SYSCALL_ARG0(ctx);
    DBG_STRACE("[PID: %d | TID %d] RMDIR_SYSCALL(%s)\n", thread->process->pid, thread->id, path);
    
    if (path == NULL) {
        return SYSCALL_ERROR;
    }
    
    int ret = vfs_remove(thread->process->fs, path, 1);
    if (ret < 0) {
        return SYSCALL_ERROR;
    }
    return SYSCALL_SUCCESS;
}

int64_t creat_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    char * path = (char *)SYSCALL_ARG0(ctx);
    int mode = SYSCALL_ARG1(ctx);
    DBG_STRACE("[PID: %d | TID %d] CREAT_SYSCALL(%s,%d)\n", thread->process->pid, thread->id, path, mode);
    
    if (path == NULL) {
        return SYSCALL_ERROR;
    }
    
    int fd = vfs_file_creat(thread->process->fs, path, mode);
    if (fd < 0) {
        return SYSCALL_ERROR;
    }
    
    return add_open_file(thread->process, fd);
}

int64_t unlinkat_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    char * path = (char *)SYSCALL_ARG0(ctx);
    int flags = SYSCALL_ARG1(ctx);
    DBG_STRACE("[PID: %d | TID %d] UNLINKAT_SYSCALL(%s,%d)\n", thread->process->pid, thread->id, path, flags);
    DBG_WARN("UNLINKAT IS THE SAME AS RMDIR, WE ARE IGNORING AT\n");
    if (path == NULL) {
        return SYSCALL_ERROR;
    }
    
    int ret = vfs_remove(thread->process->fs, path, flags);
    if (ret < 0) {
        return SYSCALL_ERROR;
    }
    return SYSCALL_SUCCESS;
}

int64_t rename_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    char * oldpath = (char *)SYSCALL_ARG0(ctx);
    char * newpath = (char *)SYSCALL_ARG1(ctx);
    DBG_STRACE("[PID: %d | TID %d] RENAME_SYSCALL(%s,%s)\n", thread->process->pid, thread->id, oldpath, newpath);
    
    if (oldpath == NULL || newpath == NULL) {
        return SYSCALL_ERROR;
    }
    
    int ret = vfs_rename(thread->process->fs, oldpath, newpath);
    if (ret < 0) {
        return SYSCALL_ERROR;
    }
    return SYSCALL_SUCCESS;
}

int64_t renameat_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    char * oldpath = (char *)SYSCALL_ARG0(ctx);
    char * newpath = (char *)SYSCALL_ARG1(ctx);
    int flags = SYSCALL_ARG2(ctx);
    DBG_STRACE("[PID: %d | TID %d] RENAMEAT_SYSCALL(%s,%s,%d)\n", thread->process->pid, thread->id, oldpath, newpath, flags);
    DBG_WARN("RENAME IS THE SAME AS RENAMEAT\n");
    if (oldpath == NULL || newpath == NULL) {
        return SYSCALL_ERROR;
    }
    
    int ret = vfs_rename(thread->process->fs, oldpath, newpath);
    if (ret < 0) {
        return SYSCALL_ERROR;
    }
    return SYSCALL_SUCCESS;
}

int64_t readdir_syscall_handler(thread_t * thread, cpu_context_t * ctx) {
    int handle = SYSCALL_ARG0(ctx);
    void * buffer = (char *)SYSCALL_ARG1(ctx);
    uint32_t * count = (uint32_t *)SYSCALL_ARG2(ctx);

    int pfd = get_open_file(thread->process, handle);
    if (pfd < 0) {
        DBG_ERROR("Invalid file descriptor %d\n", handle);
        return SYSCALL_ERROR;
    }

    int size_read = vfs_dir_read(pfd, buffer, count);
    if (size_read < 0) {
        return SYSCALL_ERROR;
    }

    return size_read;
}

int64_t close_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    int fd = SYSCALL_ARG0(ctx);
    DBG_STRACE("[PID: %d | TID %d] CLOSE_SYSCALL(%d)\n", thread->process->pid, thread->id, fd);
    
    int pfd = get_open_file(thread->process, fd);
    if (pfd < 0) {
        DBG_ERROR("File descriptor not found\n");
        return SYSCALL_ERROR;
    }

    int ret = vfs_file_close(pfd);
    if (ret < 0) {
        DBG_ERROR("Could not close file descriptor %d\n", pfd);
        return SYSCALL_ERROR;
    }

    remove_open_file(thread->process, fd);
    DBG_DEBUG("File descriptor %d closed successfully\n", fd);
    return SYSCALL_SUCCESS;
}



int64_t sigreturn_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    DBG_STRACE("[PID: %d | TID %d] SIGRETURN_SYSCALL()\n", thread->process->pid, thread->id);
    restore_signal_context(thread, ctx);
}

int64_t seek_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    int fd = SYSCALL_ARG0(ctx);
    int offset = SYSCALL_ARG1(ctx);
    int whence = SYSCALL_ARG2(ctx);
    DBG_STRACE("[PID: %d | TID %d] SEEK_SYSCALL(%d,%d,%d)\n", thread->process->pid, thread->id, fd, offset, whence);
    
    if (fd < 0) {
        return SYSCALL_ERROR;
    }
    
    int pfd = get_open_file(thread->process, fd);
    int ret = vfs_file_seek(pfd, offset, whence);
    if (ret < 0) {
        return SYSCALL_ERROR;
    }
    return ret;
}

int64_t stat_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    char * path = SYSCALL_ARG0(ctx);
    //size_t path_len = SYSCALL_ARG1(ctx);
    int flags = SYSCALL_ARG2(ctx);
    stat_t* stat = SYSCALL_ARG3(ctx);
    DBG_STRACE("[PID: %d | TID %d] STAT_SYSCALL(%d,%d)\n", thread->process->pid, thread->id, path, stat);
    if (flags != 0 && flags != AT_SYMLINK_NOFOLLOW) {
        DBG_ERROR("Flags not supported in stat syscall\n");
        return SYSCALL_ERROR;
    }

    int mode = O_RDONLY;
    if (flags & AT_SYMLINK_NOFOLLOW) {
        mode |= O_NOFOLLOW;
    }

    int fd = vfs_file_open(thread->process->fs, (char*)path, mode, 0);
    if (fd < 0) {
        return SYSCALL_ERROR;
    }
    
    int ret = vfs_file_stat(fd, stat);
    if (ret < 0) {
        return SYSCALL_ERROR;
    }

    vfs_file_close(fd);
    DBG_DEBUG("File size: %d\n", stat->st_size);
    DBG_DEBUG("File mode: %d\n", stat->st_mode);
    return SYSCALL_SUCCESS;
}

int64_t statx_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    int dirfd = SYSCALL_ARG0(ctx);
    char * pathname = (char *)SYSCALL_ARG1(ctx);
    int flags = SYSCALL_ARG2(ctx);
    unsigned int mask = SYSCALL_ARG3(ctx);
    struct statx* statx = (struct statx*)SYSCALL_ARG4(ctx);
    DBG_STRACE("[PID: %d | TID %d] STATX_SYSCALL()\n", thread->process->pid, thread->id);
    int pfd;
    (void)mask; //We ignore the mask for now
    if (statx == NULL) {
        DBG_ERROR("Invalid statx pointer\n");
        return SYSCALL_ERROR;
    }

    if (flags & AT_NO_AUTOMOUNT || flags & AT_STATX_SYNC_AS_STAT || flags & AT_STATX_DONT_SYNC || flags & AT_STATX_FORCE_SYNC) {
        DBG_ERROR("Flags not supported in statx syscall\n");
        return SYSCALL_ERROR;
    }

    if (pathname == NULL) {
        if (flags & AT_EMPTY_PATH) {
            pfd = get_open_file(thread->process, dirfd);
        } else {
            DBG_ERROR("Invalid pathname\n");
            return SYSCALL_ERROR;
        }
    }

    int open_flags = O_RDONLY;
    if (flags & AT_SYMLINK_NOFOLLOW) {
        open_flags |= O_NOFOLLOW;
    }

    if (pathname[0] == '/') {
        DBG_DEBUG("Absolute path: %s\n", pathname);
        struct vfs_struct vfs;
        vfs.root = thread->process->fs->root;
        vfs.pwd = thread->process->fs->root;
        pfd = vfs_file_open(&vfs, pathname, open_flags, 0);
    } else {
        if (dirfd == AT_FDCWD) {
            pfd = vfs_file_open(thread->process->fs, pathname, open_flags, 0);
        } else {
            DBG_ERROR("Relative statx not implemented yet\n");
            return SYSCALL_ERROR;
        }
    }

    if (pfd < 0) {
        DBG_ERROR("Could not open file descriptor %d\n", pfd);
        return SYSCALL_ERROR;
    }
    stat_t stat;
    int ret = vfs_file_stat(pfd, &stat);
    if (ret < 0) {
        DBG_ERROR("Could not get file stat\n");
        vfs_file_close(pfd);
        return SYSCALL_ERROR;
    }
    vfs_file_close(pfd);

    memset(statx, 0, sizeof(struct statx));
    statx->stx_mode = stat.st_mode;
    statx->stx_ino = stat.st_ino;
    statx->stx_uid = stat.st_uid;
    statx->stx_gid = stat.st_gid;
    statx->stx_atime.tv_sec = stat.st_atim.tv_sec;
    statx->stx_atime.tv_nsec = 0;
    statx->stx_mtime.tv_sec = stat.st_mtim.tv_sec;
    statx->stx_mtime.tv_nsec = 0;
    statx->stx_ctime.tv_sec = stat.st_ctim.tv_sec;
    statx->stx_ctime.tv_nsec = 0;
    statx->stx_size = stat.st_size;
    statx->stx_nlink = stat.st_nlink;
    statx->stx_blksize = stat.st_blksize; // Typical block size
    statx->stx_blocks = (stat.st_size + statx->stx_blksize - 1) / statx->stx_blksize; // Calculate number of blocks

    //Set the mask
    statx->stx_mask = STATX_BASIC_STATS;

    return SYSCALL_SUCCESS;

}

int64_t sigprocmask_syscall_handler(thread_t* thread, cpu_context_t* ctx) {
    int how = SYSCALL_ARG0(ctx);
    sigset_t* set = (sigset_t*)SYSCALL_ARG1(ctx);
    sigset_t* oldset = (sigset_t*)SYSCALL_ARG2(ctx);
    DBG_STRACE("[PID: %d | TID %d] SIGPROCMASK_SYSCALL(%d,%d,%d)\n", thread->process->pid, thread->id, how, set, oldset);
    if (how != SIG_BLOCK && how != SIG_UNBLOCK && how != SIG_SETMASK) {
        DBG_ERROR("Invalid how value\n");
        return SYSCALL_ERROR;
    }
    return sigprocmask(&(thread->sigprocmask), how, set, oldset);
}

int64_t sigaction_syscall_handler(thread_t* thread, cpu_context_t* ctx) {
    int signum = SYSCALL_ARG0(ctx);
    struct sigaction* act = (struct sigaction*)SYSCALL_ARG1(ctx);
    struct sigaction* oldact = (struct sigaction*)SYSCALL_ARG2(ctx);
    DBG_STRACE("[PID: %d | TID %d] SIGACTION_SYSCALL(%d,%d,%d)\n", thread->process->pid, thread->id, signum, act, oldact);
    if (signum < 0 || signum >= NSIG) {
        DBG_ERROR("Invalid signal number\n");
        return SYSCALL_ERROR;
    }
    return sigaction(thread->process->signal_handlers, signum, act, oldact);
}

int64_t sigsuspend_syscall_handler(thread_t* thread, cpu_context_t* ctx) {
    sigset_t* mask = (sigset_t*)SYSCALL_ARG0(ctx);
    DBG_STRACE("[PID: %d | TID %d] SIGSUSPEND_SYSCALL(%d)\n", thread->process->pid, thread->id, mask);
    if (mask == NULL) {
        DBG_ERROR("Invalid mask\n");
        return SYSCALL_ERROR;
    }
    int64_t ret = sigsuspend(&(thread->sigsuspend_mask), mask);
    sched();
    return ret;
}

int64_t pread_syscall_handler(thread_t* thread, cpu_context_t* ctx) {
    int fd = (int)SYSCALL_ARG0(ctx);
    uint64_t offset = SYSCALL_ARG3(ctx);

    int pfd = get_open_file(thread->process, fd);

    int64_t current_offset = vfs_file_tell(pfd);
    vfs_file_seek(pfd, offset, 0x0);
    uint64_t result = read_syscall_handler(thread, ctx);
    vfs_file_seek(pfd, current_offset, 0x0);
    return result;
}

int64_t sigpending_syscall_handler(thread_t* thread, cpu_context_t* ctx) {
    sigset_t* set = (sigset_t*)SYSCALL_ARG0(ctx);
    DBG_STRACE("[PID: %d | TID %d] SIGPENDING_SYSCALL(%d)\n", thread->process->pid, thread->id, set);
    if (set == NULL) {
        DBG_ERROR("Invalid set\n");
        return SYSCALL_ERROR;
    }
    return sigpending(thread->process->signal_queue, set);
}

int64_t sigaltstack_syscall_handler(thread_t* thread, cpu_context_t* ctx) {
    struct stack* ss = (struct stack*)SYSCALL_ARG0(ctx);
    struct stack* old_ss = (struct stack*)SYSCALL_ARG1(ctx);
    DBG_STRACE("[PID: %d | TID %d] SIGALTSTACK_SYSCALL(%d,%d)\n", thread->process->pid, thread->id, ss, old_ss);
    if (ss == NULL) {
        DBG_ERROR("Invalid stack\n");
        return SYSCALL_ERROR;
    }
    struct stack stack;
    stack.base = thread->altstack_base;
    stack.top = thread->altstack;
    stack.flags = thread->altstack_flags;
    stack.size = thread->altstack_size;
    stack.guard_size = thread->altstack_guard_size;
    int64_t ret= sigaltstack(&stack, ss, old_ss);
    thread->altstack = stack.base;
    thread->altstack_base = stack.base;
    thread->altstack_flags = stack.flags;
    thread->altstack_size = stack.size;
    thread->altstack_guard_size = stack.guard_size;
    return ret;
}

int64_t kill_syscall_handler(thread_t* thread, cpu_context_t* ctx) {
    int pid = SYSCALL_ARG0(ctx);
    int signal = SYSCALL_ARG1(ctx);
    DBG_STRACE("[PID: %d | TID %d] KILL_SYSCALL(%d,%d)\n", thread->process->pid, thread->id, pid, signal);
    if (pid < 0 || signal < 0 || signal >= NSIG) {
        DBG_ERROR("Invalid pid or signal\n");
        return SYSCALL_ERROR;
    }
    struct task_signal ** squeue = get_process_by_pid(pid)->signal_queue;
    if (squeue == NULL) {
        DBG_ERROR("No such process\n");
        return SYSCALL_ERROR;
    }
    return kill(squeue, signal);
}

int64_t fstat_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    int fd = SYSCALL_ARG0(ctx);
    int flags = SYSCALL_ARG1(ctx);
    stat_t* stat = SYSCALL_ARG2(ctx);

    DBG_STRACE("[PID: %d | TID %d] FSTAT_SYSCALL(%d,%d)\n", thread->process->pid, thread->id, fd, stat);
    
    if (flags != 0 && flags != AT_SYMLINK_NOFOLLOW) {
        DBG_ERROR("Flags not supported in fstat syscall\n");
        return SYSCALL_ERROR;
    }

    int pfd = get_open_file(thread->process, fd);
    int ret = vfs_file_stat(pfd, stat);
    if (ret < 0) {
        return SYSCALL_ERROR;
    }
    DBG_DEBUG("File size: %d\n", stat->st_size);
    DBG_DEBUG("File mode: %d\n", stat->st_mode);

    return SYSCALL_SUCCESS;
}

int64_t ioctl_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    uint64_t fd = SYSCALL_ARG0(ctx);
    uint64_t request = SYSCALL_ARG1(ctx);
    uint64_t arg = SYSCALL_ARG2(ctx);
    DBG_STRACE("[PID: %d | TID %d] IOCTL_SYSCALL(%d,%d,%d)\n", thread->process->pid, thread->id, fd, request, arg);
    int pfd = get_open_file(thread->process, fd);
    int ret = vfs_file_ioctl(pfd, request, arg);
    if (ret < 0) {
        return SYSCALL_ERROR;
    }

    DBG_DEBUG("IOCTL request: %d\n", request);
    return ret;
}

int64_t clock_gettime_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    uint64_t clock_id = SYSCALL_ARG0(ctx);
    struct timespec * ts = (struct timespec *)SYSCALL_ARG1(ctx);

    if (clock_id != CLOCK_MONOTONIC) {
        DBG_ERROR("Invalid clock id\n");
        return SYSCALL_ERROR;
    }
    DBG_STRACE("[PID: %d | TID %d] CLOCK_GETTIME_SYSCALL(%d,%d)\n", thread->process->pid, thread->id, clock_id, ts);
    if (ts != NULL) {
        timespec_now(ts);
    }

    return SYSCALL_SUCCESS;
}

int64_t clock_settime_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    (void)thread;
    DBG_ERROR("Not implemented yet\n");
    return SYSCALL_ERROR;
}

int64_t clock_getres_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    uint64_t clock_id = SYSCALL_ARG0(ctx);
    struct timespec * ts = (struct timespec *)SYSCALL_ARG1(ctx);

    if (clock_id != CLOCK_MONOTONIC) {
        DBG_ERROR("Invalid clock id\n");
        return SYSCALL_ERROR;
    }
    DBG_STRACE("[PID: %d | TID %d] CLOCK_GETRES_SYSCALL(%d,%d)\n", thread->process->pid, thread->id, clock_id, ts);
    if (ts != NULL) {
        clock_res(ts);
    }
    return SYSCALL_SUCCESS;
}



int64_t gettimeofday_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    struct timeval *tv = (struct timeval *)SYSCALL_ARG0(ctx);
    struct timezone *tz = (struct timezone *)SYSCALL_ARG1(ctx);
    DBG_STRACE("[PID: %d | TID %d] GETTIMEOFDAY_SYSCALL(%d,%d)\n", thread->process->pid, thread->id, tv, tz);
    if (tv != NULL) {
        timeval_now(tv);
    }
    return SYSCALL_SUCCESS;
}

int64_t fcntl_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    DBG_STRACE("[PID: %d | TID %d] FCNTL NOT IMPLEMENTED!!!\n", thread->process->pid, thread->id);
    return SYSCALL_SUCCESS;
}

int64_t waitpid_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    int pid = SYSCALL_ARG0(ctx);
    int * status = (int *)SYSCALL_ARG1(ctx);
    int options = SYSCALL_ARG2(ctx);
    DBG_STRACE("[PID: %d | TID %d] WAITPID_SYSCALL(%d,%d,%d)\n", thread->process->pid, thread->id, pid, status, options);
    
    if (pid < -1) {
        DBG_ERROR("Invalid pid\n");
        return SYSCALL_ERROR;
    }

    if (pid == 0) {
        DBG_ERROR("Invalid pid\n");
        return SYSCALL_ERROR;
    }

    int result = waitpid(thread, pid, status, options);
    if (result == -2) {
        return get_current_thread()->user_context->cpu_context->rax;
    } else {
        return result;
    }
}   

int64_t sched_yield_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    DBG_STRACE("[PID: %d | TID %d] SCHED_YIELD_SYSCALL()\n", thread->process->pid, thread->id);
    DBG_DEBUG("Yielding process %d\n", get_current_process()->pid);
    sched();
    DBG_DEBUG("Resuming process %d\n", get_current_process()->pid);
    return SYSCALL_SUCCESS;
}

int64_t fork_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    DBG_STRACE("[PID: %d | TID %d] FORK_SYSCALL()\n", thread->process->pid, thread->id);
    uint64_t child_pid = (uint64_t)fork(thread);
    DBG_DEBUG("Child PID: %d | TID %d\n", child_pid, 0);
    return child_pid;
}

int64_t execve_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    const char * path = (const char *)SYSCALL_ARG0(ctx);
    const char ** argv = (const char **)SYSCALL_ARG1(ctx);
    const char ** envp = (const char **)SYSCALL_ARG2(ctx);
    if (path == NULL) {
        DBG_ERROR("Invalid arguments for execve\n");
        return SYSCALL_ERROR;
    } else if (argv == NULL || envp == NULL) {
        DBG_STRACE("[PID: %d | TID %d] EXECVE_SYSCALL(%s,NULL,NULL)\n", thread->process->pid, thread->id, path);
    } else {
        DBG_STRACE("[PID: %d | TID %d] EXECVE_SYSCALL(%s,%s,%s)\n", thread->process->pid, thread->id, path, argv, envp);
    }
    if (execve(thread->process, path, argv, envp) != 0) {
        return SYSCALL_ERROR;
    }
    return SYSCALL_SUCCESS;
}

int64_t arch_prctl_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    (void)thread;
    uint64_t option = SYSCALL_ARG0(ctx);
    uint64_t arg2 = SYSCALL_ARG1(ctx);
    DBG_STRACE("[PID: %d | TID %d] PRCTL_SYSCALL(%llx,%llx)\n", thread->process->pid, thread->id, option, arg2);
    switch (option) {
        case ARCH_SET_CPUID:
            return -ENODEV;
        case ARCH_GET_CPUID:
            return -ENODEV;
        case ARCH_SET_FS:
            thread->user_context->fs_base = (uint64_t)arg2;
            break;
        case ARCH_GET_FS: {
            unsigned long * fs_base = (unsigned long *)(unsigned long)arg2;
            if (fs_base == NULL) {
                return -EINVAL;
            }

            *fs_base = (unsigned long)thread->user_context->fs_base;
            setFsBase(thread->user_context->fs_base);
            break;
        }
        case ARCH_SET_GS:
        thread->user_context->gs_base = (uint64_t)arg2;
            break;
        case ARCH_GET_GS: {
            unsigned long * gs_base = (unsigned long *)(unsigned long)arg2;
            if (gs_base == NULL) {
                return -EINVAL;
            }

            *gs_base = (unsigned long)thread->user_context->gs_base;
            break;
        }
        default:
            return -EINVAL;
    }
    return SYSCALL_SUCCESS;
}

int64_t thread_exit_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    DBG_STRACE("[PID: %d | TID %d] THREAD_EXIT_SYSCALL()\n", thread->process->pid, thread->id);
    thread_exit(thread);
    return SYSCALL_SUCCESS;
}

int64_t exit_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    int error_code = SYSCALL_ARG0(ctx);
    DBG_STRACE("[PID: %d | TID %d] EXIT_SYSCALL(%d)\n", thread->process->pid, thread->id, error_code);
    exit(thread->process, error_code);
    return SYSCALL_SUCCESS;
}

int64_t get_tid_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    //DBG_STRACE("[PID: %d | TID %d] GET_TID_SYSCALL()\n", thread->process->pid, thread->id);
    return thread->id;
}

int64_t getpid_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    DBG_STRACE("[PID: %d | TID %d] GETPID_SYSCALL()\n", thread->process->pid, thread->id);
    return thread->process->pid;
}

int64_t getppid_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    DBG_STRACE("[PID: %d | TID %d] GETPPID_SYSCALL()\n", thread->process->pid, thread->id);
    if (thread->process->parent == NULL) {
        DBG_ERROR("No parent process\n");
        return 1;
    }
    return thread->process->parent->pid;
}

int64_t mkdir_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    char * path = (char *)SYSCALL_ARG0(ctx);
    int mode = SYSCALL_ARG1(ctx);

    DBG_STRACE("[PID: %d | TID %d] MKDIR_SYSCALL(%s,%d)\n", thread->process->pid, thread->id, path, mode);
    int ret = vfs_mkdir(thread->process->fs, path, mode);
    if (ret < 0) {
        return SYSCALL_ERROR;
    }
    return SYSCALL_SUCCESS;
}

int64_t log_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    char * message = (char *)SYSCALL_ARG0(ctx);
    //uint64_t length = SYSCALL_ARG1(ctx);
    //enable_debugger();
    //DBG_STRACE("[PID: %d | TID %d] LOG_SYSCALL(%s,%d)\n", thread->process->pid, thread->id, message, length);
    //uint8_t dbg = is_debugger_enabled();
    //if (!dbg) enable_debugger();
    DBG_INFO("%s\n", message);
    //if (!dbg) disable_debugger();
    //disable_debugger();
    return SYSCALL_SUCCESS;
}

//void * mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
#define PROT_NONE  0x00
#define PROT_READ  0x01
#define PROT_WRITE 0x02
#define PROT_EXEC  0x04

#define MAP_FAILED ((void *)(-1))
#define MAP_FILE    0x00
#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANON      0x20
#define MAP_ANONYMOUS 0x20
#define MAP_GROWSDOWN 0x100
#define MAP_DENYWRITE 0x800
#define MAP_EXECUTABLE 0x1000
#define MAP_LOCKED    0x2000
#define MAP_NORESERVE 0x4000
#define MAP_POPULATE  0x8000
#define MAP_NONBLOCK  0x10000
#define MAP_STACK     0x20000
#define MAP_HUGETLB   0x40000
#define MAP_SYNC      0x80000
#define MAP_FIXED_NOREPLACE 0x100000
int64_t mmap_syscall_handler(thread_t*thread, cpu_context_t*ctx) {
    void * addr = (void *)SYSCALL_ARG0(ctx);
    uint64_t length = SYSCALL_ARG1(ctx);
    int prot = SYSCALL_ARG2(ctx);
    int flags = SYSCALL_ARG3(ctx);
    int fd = SYSCALL_ARG4(ctx);
    off_t offset = SYSCALL_ARG5(ctx);
    DBG_STRACE("[PID: %d | TID %d] MMAP_SYSCALL(%p,%d,%d,%d,%d,%d)\n", thread->process->pid, thread->id, addr, length, prot, flags, fd, offset);
    int pfd = get_open_file(thread->process, fd);
    //Validate the arguments
    if (length == 0) {
        panic("Length is 0\n");
        return SYSCALL_ERROR;
    }
    //Make sure MAP_SHARED, MAP_PRIVATE
    if ((flags & MAP_SHARED) && (flags & MAP_PRIVATE)) {
        panic("MAP_SHARED and MAP_PRIVATE are used together\n");
        return SYSCALL_ERROR;
    }
    
    //Check that protections are valid
    uint8_t vmm_flags = 0;
    if (!(prot & PROT_READ) || prot & PROT_NONE) {
        panic("Not implemented!");
    }
    if (prot & PROT_WRITE) {
        vmm_flags |= VMM_WRITE_BIT;
    }
    if (!(prot & PROT_EXEC)) {
        vmm_flags |= VMM_NX_BIT;
    }
    vmm_flags |= VMM_USER_BIT;

    uint8_t vma_flags = 0;
    if (flags & MAP_SHARED) {
        vma_flags |= VMAREA_EXT_SHARED;
    }
    if (flags & MAP_PRIVATE) {
        vma_flags |= VMAREA_EXT_COW;
    }
    if (flags & PROT_NONE || (!(flags & PROT_READ) && !(flags & PROT_WRITE))) {
        vma_flags |= VMAREA_EXT_GUARD;
    }

    if (pfd < 0 && pfd != -1) {
        panic("Invalid pfd\n");
        return SYSCALL_ERROR;
    }
    if (offset % PAGE_SIZE != 0) {
        panic("Offset is not page aligned\n");
        return SYSCALL_ERROR;
    }
    if (offset > 0 && pfd == 0) {
        panic("Invalid pfd\n");
        return SYSCALL_ERROR;
    }

    if (addr != NULL && ((uint64_t)addr % PAGE_SIZE != 0))
        addr = (void *)((uint64_t)addr & ~(PAGE_SIZE - 1));
    addr = find_shm_vmarea(thread->process, addr, length);
    if (addr == NULL) {
        panic("Failed to find a free area\n");
        return SYSCALL_ERROR;
    }
    //DBG_DEBUG("Found free area: %p\n", addr);

    if (flags & MAP_PRIVATE || flags & MAP_SHARED) {

        allocate_at_vaddr(thread->process->vmm, addr, length, vmm_flags);

        int newfd = -1;
        if (flags & MAP_ANONYMOUS) {
            create_vmarea(thread->process, addr, (addr + length), vmm_flags, vma_flags, PAGE_SIZE_4KIB, newfd, 0);
            //DBG_DEBUG("MMAP <anon> Giving range: %p-%p, length: %d, prot: %d, flags: %d, fd: %d, offset: %d\n", addr, (void*)(((uint64_t)addr)+length), length, prot, flags, newfd, offset);
            return addr;
        } else {
            newfd = vfs_file_dup(pfd, -1);
            if (newfd < 0) {
                goto cleanup_on_error;
            }
        }

        create_vmarea(thread->process, addr, (addr + length), vmm_flags, vma_flags, PAGE_SIZE_4KIB, newfd, offset);
        
        //add write privilege to the buffer
        mprotect(thread->process->vmm, addr, length, PROT_READ | PROT_WRITE);

        //Read the file into the memory
        if (vfs_file_seek(newfd, offset, SEEK_SET) < 0) {
            DBG_ERROR("Failed to seek file\n");
            vfs_file_close(newfd);
            goto cleanup_on_error;
        }

        int64_t bytes_read = vfs_file_read(newfd, addr, length);
        if (bytes_read < 0) {
            DBG_ERROR("Failed to read file\n");
            vfs_file_close(newfd);
            goto cleanup_on_error;
        } 

        //Reset permissions but keep readonly so it page faults on a write
        uint8_t roflags = vmm_flags & ~VMM_WRITE_BIT;   
        mprotect(thread->process->vmm, addr, length, roflags);
        //DBG_DEBUG("MMAP Giving range: %p-%p, length: %d, prot: %d, flags: %d, fd: %d, offset: %d\n", addr, (void*)(((uint64_t)addr)+length), length, prot, flags, newfd, offset);
        return addr;
    }

cleanup_on_error:
    unmap_range(thread->process->vmm, addr, length);
    remove_vmarea(thread->process, addr);
    panic("MMAP ERROR\n");
    return SYSCALL_ERROR;
}

//mprotect
int64_t mprotect_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    void * addr = (void *)SYSCALL_ARG0(ctx);
    size_t length = SYSCALL_ARG1(ctx);
    int prot = SYSCALL_ARG2(ctx);

    DBG_STRACE("[PID: %d | TID %d] MPROTECT_SYSCALL(%p,%d,%d)\n", thread->process->pid, thread->id, addr, length, prot);
    if (addr == NULL || length == 0) {
        return SYSCALL_ERROR;
    }

    //Check that protections are valid
    uint8_t vmm_flags = 0;
    if (!(prot & PROT_READ) || prot & PROT_NONE) {
        panic("Not implemented!");
    }

    if (prot & PROT_WRITE) {
        vmm_flags |= VMM_WRITE_BIT;
    }
    if (!(prot & PROT_EXEC)) {
        vmm_flags |= VMM_NX_BIT;
    }
    vmm_flags |= VMM_USER_BIT;
    if (prot & PROT_NONE) {
        vmm_flags |= VMM_NX_BIT;
    }
    
    //Check if the address is in a vmarea
    struct vm_area * vma = is_in_vmarea(thread->process, addr);
    if (vma == NULL) {
        panic("Failed to find vmarea\n");
    }

    if (vma->start != addr) {
        panic("Invalid address\n");
    }

    if (length > (uint64_t)(vma->end - vma->start)) {
        panic("Length is greater than vmarea\n");
    }

    mprotect(thread->process->vmm, addr, length, vmm_flags);
    return NULL;
}

int64_t munmap_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    void * addr = (void *)SYSCALL_ARG0(ctx);
    size_t length = SYSCALL_ARG1(ctx);
    DBG_STRACE("[PID: %d | TID %d] MUNMAP_SYSCALL(%p,%d)\n", thread->process->pid, thread->id, addr, length);
    
    if (addr == NULL || length == 0) {
        return SYSCALL_ERROR;
    }
    
    //Unmap the memory
    struct vm_area * vma = is_in_vmarea(thread->process, addr);
    if (vma == NULL) {
        panic("Failed to find vmarea\n");
    }

    if (vma->start != addr) {
        panic("Invalid address\n");
    }

    if (vma->extended_flags & VMAREA_EXT_REQ_SYNC) {
        vmarea_sync(vma, 0);
        //Write to the file if necessary
    }

    remove_vmarea(thread->process, addr);
    unmap_range(thread->process->vmm, addr, length);
    return SYSCALL_SUCCESS;
}

#define MS_SYNC 0x0
#define MS_ASYNC 0x1
#define MS_INVALIDATE 0x2
int64_t msync_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    void * addr = (void *)SYSCALL_ARG0(ctx);
    size_t length = SYSCALL_ARG1(ctx);
    int flags = SYSCALL_ARG2(ctx);
    DBG_STRACE("[PID: %d | TID %d] MSYNC_SYSCALL(%p,%d,%d)\n", thread->process->pid, thread->id, addr, length, flags);

    if (flags & MS_ASYNC) {
        panic("MS_ASYNC not implemented\n");
    }
    if (flags & MS_INVALIDATE) {
        panic("MS_INVALIDATE not implemented\n");
    }

    if (addr == NULL || length == 0) {
        return SYSCALL_ERROR;
    }
    //Unmap the memory
    struct vm_area * vma = is_in_vmarea(thread->process, addr);
    if (vma == NULL) {
        panic("Failed to find vmarea\n");
    }

    if (vma->start != addr) {
        panic("Invalid address\n");
    }

    if (length > (uint64_t)(vma->end - vma->start)) {
        panic("Length is greater than vmarea\n");
    }

    if (vma->extended_flags & VMAREA_EXT_REQ_SYNC) {
        vmarea_sync(vma, length);
    }
}

int64_t nanosleep_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    struct timespec *duration = SYSCALL_ARG0(ctx);
    struct timespec *rem = SYSCALL_ARG1(ctx);
    //DBG_STRACE("[PID: %d | TID %d] NANOSLEEP_SYSCALL(%d,%d)\n", thread->process->pid, thread->id, duration, rem);
    if (duration == NULL) {
        DBG_ERROR("Invalid duration\n");
        return SYSCALL_ERROR;
    }

    nanosleep(thread, duration, rem);
}

int64_t dup_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    int fd = SYSCALL_ARG0(ctx);
    DBG_STRACE("[PID: %d | TID %d] DUP_SYSCALL(%d)\n", thread->process->pid, thread->id, fd);
    if (fd < 0) {
        return SYSCALL_ERROR;
    }

    if (thread->process->open_files_count >= MAX_OPEN_FILES) {
        DBG_ERROR("Max open files reached\n");
        return SYSCALL_ERROR;
    }

    int pfd = get_open_file(thread->process, fd);

    int newfd = vfs_file_dup(pfd, -1);
    if (newfd < 0) {
        return SYSCALL_ERROR;
    }
    return add_open_file(thread->process, newfd);
}

int64_t dup2_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    int oldfd = SYSCALL_ARG0(ctx);
    int newfd = SYSCALL_ARG1(ctx);
    DBG_STRACE("[PID: %d | TID %d] DUP2_SYSCALL(%d,%d)\n", thread->process->pid, thread->id, oldfd, newfd);
    if (oldfd < 0 || newfd < 0) {
        return SYSCALL_ERROR;
    }

    if (thread->process->open_files_count >= MAX_OPEN_FILES) {
        DBG_ERROR("Max open files reached\n");
        return SYSCALL_ERROR;
    }
    int pfd = get_open_file(thread->process, oldfd);
    int ret = vfs_file_dup(pfd, newfd);
    if (ret < 0) {
        return SYSCALL_ERROR;
    }
    return add_open_file(thread->process, ret);
}

int64_t disable_debugger_syscall_handler(thread_t*thread, cpu_context_t* ctx) {

    uint64_t enable = SYSCALL_ARG0(ctx);
    DBG_STRACE("[PID: %d | TID %d] DISABLE_DEBUGGER_SYSCALL()\n", thread->process->pid, thread->id);
    set_debug_level((uint8_t)enable);
    return SYSCALL_SUCCESS;
}


int64_t undefined_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    DBG_STRACE("[PID: %d | TID %d] UNDEFINED_SYSCALL(%d)\n", thread->process->pid, thread->id, ctx->rax);
    return SYSCALL_UNDEFINED;
}

syscall_handler syscall_handlers[SYSCALL_HANDLER_COUNT] = {
    [0] = read_syscall_handler,
    [1] = write_syscall_handler,
    [2] = open_syscall_handler,
    [3] = close_syscall_handler,
    [4] = stat_syscall_handler,
    [5] = fstat_syscall_handler,
    [6 ... 7] = undefined_syscall_handler,
    [8] = seek_syscall_handler,
    [9] = mmap_syscall_handler,
    [10] = mprotect_syscall_handler,
    [11] = munmap_syscall_handler,
    [12] = undefined_syscall_handler,
    [13] = sigaction_syscall_handler,
    [14] = sigprocmask_syscall_handler,
    [15] = sigreturn_syscall_handler,
    [16] = ioctl_syscall_handler,
    [17] = pread_syscall_handler,
    [18 ... 23] = undefined_syscall_handler,
    [24] = sched_yield_syscall_handler,
    [25] = undefined_syscall_handler,
    [26] = msync_syscall_handler,
    [27 ... 31] = undefined_syscall_handler,
    [32] = dup_syscall_handler,
    [33] = dup2_syscall_handler,
    [34] = undefined_syscall_handler,
    [35] = nanosleep_syscall_handler,
    [36 ... 38] = undefined_syscall_handler,
    [39] = getpid_syscall_handler,
    [40 ... 56] = undefined_syscall_handler,
    [57] = fork_syscall_handler,
    [58] = undefined_syscall_handler,
    [59] = execve_syscall_handler,
    [60] = exit_syscall_handler,
    [61] = waitpid_syscall_handler,
    [62] = kill_syscall_handler,
    [63 ... 71] = undefined_syscall_handler,
    [72] = fcntl_syscall_handler,
    [73 ... 78] = undefined_syscall_handler,
    [79] = getcwd_syscall_handler,
    [80] = chdir_syscall_handler,
    [81] = undefined_syscall_handler,
    [82] = rename_syscall_handler,
    [83] = mkdir_syscall_handler,
    [84] = rmdir_syscall_handler,
    [85] = creat_syscall_handler,
    [86 ... 95] = undefined_syscall_handler,
    [96] = gettimeofday_syscall_handler,
    [97 ... 109] = undefined_syscall_handler,
    [110] = getppid_syscall_handler,
    [111 ... 126] = undefined_syscall_handler,
    [127] = sigpending_syscall_handler,
    [128 ... 129] = undefined_syscall_handler,
    [130] = sigsuspend_syscall_handler,
    [131] = sigaltstack_syscall_handler,
    [132 ... 157] = undefined_syscall_handler,
    [158] = arch_prctl_syscall_handler,
    [159 ... 185] = undefined_syscall_handler,
    [186] = get_tid_syscall_handler,
    [187 ... 226] = undefined_syscall_handler,
    [227] = clock_settime_syscall_handler,
    [228] = clock_gettime_syscall_handler,
    [229] = clock_getres_syscall_handler,
    [230 ... 262] = undefined_syscall_handler,
    [263] = unlinkat_syscall_handler,
    [264] = renameat_syscall_handler,
    [265 ... 269] = undefined_syscall_handler,
    [270] = pselect_syscall_handler,
    [271 ... 331] = undefined_syscall_handler,
    [332] = statx_syscall_handler,
    [333] = undefined_syscall_handler,
    [334] = undefined_syscall_handler,
    [335] = disable_debugger_syscall_handler,
    [336] = thread_exit_syscall_handler,
    [337] = log_syscall_handler,
    [338] = futex_wait_syscall_handler,
    [339] = futex_wake_syscall_handler,
    [340] = dir_open_syscall_handler,
    [341] = readdir_syscall_handler,
    [342 ... 511] = undefined_syscall_handler
};

void global_syscall_handler(cpu_context_t* ctx) {

    thread_t * current_thread = get_current_thread();
    thread_t * entry_thread = current_thread;
    current_thread->syscall_ready = 1;

    //if (ctx->rax != 186 && ctx->rax != 337 && (ctx->rax != 1 || SYSCALL_ARG2(ctx) != 1))
    //    DBG_STRACE("[PID: %d | TID %d] SYSCALL(%d)\n", current_thread->process->pid, current_thread->id, ctx->rax);    
    memcpy(current_thread->user_context->cpu_context, ctx, sizeof(cpu_context_t));
    memcpy(current_thread->user_context->cpu_context->info, ctx->info, sizeof(struct cpu_context_info));
    arch_simd_save_context(current_thread->user_context->fxsave_region);

    current_thread->last_syscall_result = SYSCALL_SUCCESS;
    if (ctx->rax < SYSCALL_HANDLER_COUNT) {
        current_thread->last_syscall_result = syscall_handlers[ctx->rax](current_thread, ctx);
    } else {
        DBG_ERROR("Syscall number overflow %d\n", ctx->rax);
        current_thread->last_syscall_result = SYSCALL_ERROR;
    }

    current_thread = get_current_thread();
    if (current_thread->process != entry_thread->process) {
        DBG_DEBUG("Process changed during syscall!\n");
        //DBG_STRACE("[PID: %d | TID: %d] SYSCALL(%d) RETURNING %d\n", current_thread->process->pid, current_thread->id, ctx->rax, current_thread->last_syscall_result);
    } else {
    //if (ctx->rax != 186 && ctx->rax != 337 && (ctx->rax != 1 || SYSCALL_ARG2(ctx) != 1)) {
    //    //DBG_STRACE("[PID: %d | TID: %d] SYSCALL(%d) RETURNING %d\n", entry_thread->process->pid, entry_thread->id, ctx->rax, entry_thread->last_syscall_result);
    }
    
    if (!current_thread->syscall_ready && current_thread->kernel_context_ready)
        panic("Syscall not ready but sleep context is ready!\n");

    if (current_thread->kernel_context_ready) {
        __asm__ volatile("int $0x79");
    }

    arch_simd_restore_context(current_thread->user_context->fxsave_region);
    memcpy(ctx, current_thread->user_context->cpu_context, sizeof(cpu_context_t));
    memcpy(ctx->info, current_thread->user_context->cpu_context->info, sizeof(struct cpu_context_info));

    struct tss * tss = arch_get_cpu(current_thread->core_id)->tss;
    tss_set_stack(tss, ctx->info->kstack, 0);
    tss_set_stack(tss, ctx->rsp, 3);
    setFsBase(current_thread->user_context->fs_base);

    if (current_thread->syscall_ready) {
        int signo;
        struct sigaction * sigact = select_signal(current_thread, &signo);
        if (sigact == NULL) {
            if (current_thread->waiting == 2) {
                DBG_DEBUG("Waitpid ready to be handled [PID: %d | TID: %d]\n", current_thread->process->pid, current_thread->id);
                current_thread->waiting = 0;
                int * phys = get_physical_address(current_thread->process->vmm, current_thread->waitpid_status_address);
                if (phys == NULL) {
                    DBG_ERROR("Failed to get physical address\n");
                    SYSRET(ctx, -1);
                }
                *(int*)(to_identity_map(phys)) = current_thread->waitpid_status;
                SYSRET(ctx, current_thread->waitpid_pid);
            }
        } else {
            DBG_DEBUG("Signal ready to be handled [HANDLER: %p | SIGACTION: %p | MASK: %llx | FLAGS: %x | RESTORER: %p]\n", sigact->sa_handler, sigact->sa_sigaction, sigact->sa_mask, sigact->sa_flags, sigact->sa_restorer);
            create_signal_context(current_thread, signo, sigact, ctx);
        }

        SYSRET(ctx, current_thread->last_syscall_result);
    } else {
    __asm__("mov %0, %%rsp\n"
            "mov %1, %%cr3\n"
            "ret\n" : : "r" (current_thread->ustack), "r" (current_thread->user_context->cpu_context->cr3));
    }
}