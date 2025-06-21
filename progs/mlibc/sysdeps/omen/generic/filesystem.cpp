#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <omen/syscall.h>
#include <omen/ttycheck.h>
#include <bits/ensure.h>
#include <frg/vector.hpp>
#include <mlibc/debug.hpp>
#include <mlibc/allocator.hpp>
#include <mlibc/all-sysdeps.hpp>
#include <abi-bits/seek-whence.h>

namespace mlibc{
    int sys_open(const char *pathname, int flags, mode_t mode, int *fd){
        auto result = do_syscall(SYS_FILE_OPEN, pathname, flags, mode);

        if(result < 0){
            return -result;
        }

        *fd = result;
        return 0;
    }

    int sys_read(int fd, void *buf, size_t count, ssize_t *bytes_read){
        auto result = do_syscall(SYS_FILE_READ, fd, buf, count);

        if(result < 0){
            *bytes_read = 0;
            return -result;
        }

        *bytes_read = result;
        return 0;
    }

    int sys_write(int fd, const void *buf, size_t count, ssize_t *bytes_written){
        auto result = do_syscall(SYS_FILE_WRITE, fd, buf, count);

        if(result < 0){
            return -result;
        }

        *bytes_written = result;
        return 0;
    }

    int sys_seek(int fd, off_t offset, int whence, off_t *new_offset){
        auto result = do_syscall(SYS_FILE_SEEK, fd, offset, whence);

        if(result < 0){
            return -result;
        }

        *new_offset = result;
        return 0;
    }

    int sys_close(int fd){
        auto result = do_syscall(SYS_FILE_CLOSE, fd);

        if(result < 0){
            return -result;
        }

        return 0;
    }

    int sys_flock(int fd, int options){
        // TODO
        __ensure(!"Not implemented");
        return 0;
    }

    int sys_ioctl(int fd, unsigned long request, void* arg, int* ptr_result){
        auto result = do_syscall(SYS_FILE_IOCTL, fd, request, arg);

        if(result < 0){
            return -result;
        }

        if(ptr_result != NULL){
            *ptr_result = result;
        }
        
        return 0;
    }

    int sys_open_dir(const char *path, int *handle){
        // TODO
        auto result = do_syscall(SYS_DIR_OPEN, path);
        if(result < 0){
            return -result;
        }
        *handle = result;
        return 0;
    }

    int sys_read_entries(int handle, void *buffer, size_t max_size, size_t *bytes_read){
        // TODO
        auto result = do_syscall(SYS_DIR_READ, handle, buffer, max_size);
        if(result < 0){
            *bytes_read = 0;
            return -result;
        }
        *bytes_read = result;
        return 0;
    }

    int sys_pread(int fd, void *buf, size_t n, off_t off, ssize_t *bytes_read){
        // TODO
        auto result = do_syscall(SYS_FILE_PREAD, fd, buf, n, off);
        if(result < 0){
            *bytes_read = 0;
            return -result;
        }
        *bytes_read = result;
        return 0;
    }

    int sys_mkdir(const char *path, mode_t mode) {
	    auto result = do_syscall(SYS_MKDIR, path, mode);
        if(result < 0){
            return -result;
        }

        return 0;
    }

    // In contrast to the isatty() library function, the sysdep function uses return value
    // zero (and not one) to indicate that the file is a terminal.
    int sys_isatty(int fd){
        // TODO
        struct winsize ws;
        auto result = do_syscall(SYS_FILE_IOCTL, fd, TIOCGWINSZ, &ws);
        if (result == TTY_CHECK_VAL) return 0;
        return ENOTTY;
    }

    int sys_rmdir(const char *path){
        auto result = do_syscall(SYS_RMDIR, path, strlen(path));
        if(result < 0){
            return -result;
        }

        return 0;
    }

    int sys_unlinkat(int dirfd, const char *path, int flags){
        if(flags != 0){
            mlibc::infoLogger() << "mlibc warning: sys_unlinkat: flags not supported" << frg::endlog;
            return EINVAL;
        }

        auto result = do_syscall(SYS_UNLINKAT, dirfd, path, strlen(path));
        if(result < 0){
            return -result;
        }

        return 0;
    }

    int sys_rename(const char *path, const char *new_path){
        auto result = do_syscall(SYS_RENAMEAT, path, new_path);
        if(result < 0){
            return -result;
        }
        return 0;
    }

    int sys_renameat(int olddirfd, const char *old_path, int newdirfd, const char *new_path){
        auto result = do_syscall(SYS_RENAME, old_path, new_path);
        if(result < 0){
            return -result;
        }
        return 0;
    }

    int sys_stat(fsfd_target fsfdt, int fd, const char *path, int flags, struct stat *statbuf){
        auto result = 0;
        switch(fsfdt){
            case fsfd_target::path:{
                result = do_syscall(SYS_PATH_STAT, path, strlen(path), flags, statbuf);
                break;
            }
            case fsfd_target::fd:{
                result = do_syscall(SYS_FD_STAT, fd, flags, statbuf);
                break;
            }
            default:{
                mlibc::infoLogger() << "mlibc warning: sys_stat: unsupported fsfd target" << frg::endlog;
                return EINVAL;
            }
        }

        if(result < 0){
            return -result;
        }

        return 0;
    }

    int sys_getcwd(char* buffer, size_t size){
        auto result = do_syscall(SYS_GETCWD, buffer, size);

        if(result < 0){
            return -result;
        }

        return 0;
    }

    int sys_dup(int fd, int flags, int *newfd) {
        // TODO
        auto result = do_syscall(SYS_DUP, fd, flags);

        if(result < 0){
            return -result;
        }

        *newfd = result;
        return 0;
    }
    
    int sys_dup2(int fd, int flags, int newfd) {
        // TODO
        auto result = do_syscall(SYS_DUP2, fd, newfd, flags);

        if(result < 0){
            return -result;
        }

        return 0;
    }

    int sys_pselect(int nfds, fd_set* readfds, fd_set* writefds, fd_set *exceptfds, const struct timespec* timeout, const sigset_t* sigmask, int *num_events) {
        (void)sigmask; // sigmask is not used in this implementation
        auto result = do_syscall(SYS_PSELECT, nfds, readfds, writefds, exceptfds, timeout, num_events);

        if(result < 0){
            return -result;
        }
        
        return 0;
    }

    int sys_fcntl(int fd, int request, va_list args, int* ptr_result){
        auto result = do_syscall(SYS_FCNTL, fd, request, va_arg(args, uint64_t));

        if(result < 0){
            return -result;
        }

        if(ptr_result != NULL){
            *ptr_result = result;
        }
        
        return 0;
    }

#ifndef MLIBC_BUILDING_RTDL
    int sys_chdir(const char *path){
        auto result = do_syscall(SYS_CHDIR, path, strlen(path));

        if(result < 0){
            return -result;
        }

        return 0;
    }
#endif
}