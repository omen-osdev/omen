#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <bits/ensure.h>

/* types */
using sc_word_t = long;

/* list */
#define SYS_COUNT               32
#define SYS_FILE_READ           0
#define SYS_FILE_WRITE          1
#define SYS_FILE_OPEN           2
#define SYS_FILE_CLOSE          3
#define SYS_PATH_STAT           4
#define SYS_FD_STAT             5
#define SYS_FILE_SEEK           8
#define SYS_MMAP                9
#define SYS_MPROTECT            10
#define SYS_MUNMAP              11
#define SYS_SIGACTION           13
#define SYS_SIGPROCMASK         14
#define SYS_SIGRETURN           15
#define SYS_FILE_IOCTL          16
#define SYS_FILE_PREAD          17
#define SYS_DUP                 32
#define SYS_DUP2                33
#define SYS_NANOSLEEP           35
#define SYS_GETPID              39
#define SYS_FORK                57
#define SYS_EXECVE              59
#define SYS_EXIT                60
#define SYS_WAITPID             61
#define SYS_KILL                62
#define SYS_FCNTL               72
#define SYS_GETCWD              79
#define SYS_CHDIR               80
#define SYS_RENAME              82
#define SYS_MKDIR               83
#define SYS_RMDIR               84
#define SYS_CREAT               85
#define SYS_GETPPID             110
#define SYS_ARCH_PRCTL          158
#define SYS_GET_TID             186
#define SYS_CLOCK_GET           228
#define SYS_CLOCK_GETRES        229
#define SYS_UNLINKAT            263
#define SYS_RENAMEAT            264
#define SYS_PSELECT             270
#define SYS_STATX               332
#define SYS_THREAD_EXIT         336
#define SYS_LOG                 337
#define SYS_FUTEX_WAIT          338
#define SYS_FUTEX_WAKE          339
#define SYS_DIR_OPEN            340
#define SYS_DIR_READ            341

/* extern functions */
sc_word_t do_syscall0(long sc);
sc_word_t do_syscall1(long sc, sc_word_t arg1);
sc_word_t do_syscall2(long sc, sc_word_t arg1, sc_word_t arg2);
sc_word_t do_syscall3(long sc, sc_word_t arg1, sc_word_t arg2, sc_word_t arg3);
sc_word_t do_syscall4(long sc, sc_word_t arg1, sc_word_t arg2, sc_word_t arg3, sc_word_t arg4);
sc_word_t do_syscall5(long sc, sc_word_t arg1, sc_word_t arg2, sc_word_t arg3, sc_word_t arg4, sc_word_t arg5);
sc_word_t do_syscall6(long sc, sc_word_t arg1, sc_word_t arg2, sc_word_t arg3, sc_word_t arg4, sc_word_t arg5, sc_word_t arg6);


/* inline functions */
inline sc_word_t sc_cast(long x) { 
    return x; 
}
inline sc_word_t sc_cast(const void *x) { 
    return reinterpret_cast<sc_word_t>(x);
}

__attribute__((always_inline)) static inline long _do_syscall(int call) {
    return do_syscall0(call);
}

__attribute__((always_inline)) static inline long _do_syscall(int call, sc_word_t arg0) {
    return do_syscall1(call, arg0);
}

__attribute__((always_inline)) static inline long _do_syscall(int call, sc_word_t arg0, sc_word_t arg1) {
    return do_syscall2(call, arg0, arg1);
}

__attribute__((always_inline)) static inline long _do_syscall(int call, sc_word_t arg0, sc_word_t arg1, sc_word_t arg2) {
    return do_syscall3(call, arg0, arg1, arg2);
}

__attribute__((always_inline)) static inline long _do_syscall(int call, sc_word_t arg0, sc_word_t arg1, sc_word_t arg2, sc_word_t arg3) {
    return do_syscall4(call, arg0, arg1, arg2, arg3);
}

__attribute__((always_inline)) static inline long _do_syscall(int call, sc_word_t arg0, sc_word_t arg1, sc_word_t arg2, sc_word_t arg3, sc_word_t arg4) {
    return do_syscall5(call, arg0, arg1, arg2, arg3, arg4);
}

__attribute__((always_inline)) static inline long _do_syscall(int call, sc_word_t arg0, sc_word_t arg1, sc_word_t arg2, sc_word_t arg3, sc_word_t arg4, sc_word_t arg5) {
    return do_syscall6(call, arg0, arg1, arg2, arg3, arg4, arg5);
}

template <typename... T>
__attribute__((always_inline)) static inline long do_syscall(sc_word_t call, T... args) {
    return _do_syscall(call, sc_cast(args)...);
}

#endif // SYSCALL_H