#ifndef _PROCESS_H
#define _PROCESS_H

#include <generic/config.h>
#include <omen/hal/hal.h>
#include <omen/libraries/std/stdint.h>
#include <omen/managers/mem/vmm.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/managers/cpu/signal.h>
#include <omen/managers/mem/vdso.h>
#include <vfs/vfs.h>

#define PROTOSTACK_MAX_SIZE 0x1000

#define PROCESS_PRIORITIES                  20

#define PROCESS_STATUS_ALIVE                0
#define PROCESS_STATUS_ZOMBIE               1
#define PROCESS_STATUS_SIGSTOP              2
#define PROCESS_STATUS_SIGCONT              3

#define THREAD_STATUS_READY                 0
#define THREAD_STATUS_RUNNING               1
#define THREAD_STATUS_UNINTERRUPTIBLE_SLEEP 2
#define THREAD_STATUS_INTERRUPTIBLE_SLEEP   3
#define THREAD_STATUS_ZOMBIE                5

#define PROCFILE_STDIN                      0
#define PROCFILE_STDOUT                     1
#define PROCFILE_STDERR                     2

#define MAX_OPEN_FILES                      32
#define MAX_THREADS                         32

#define VMAREA_EXT_COW                      0x1
#define VMAREA_EXT_SHARED                   0x2
#define VMAREA_EXT_GUARD                    0x4
#define VMAREA_EXT_REQ_SYNC                 0x8
#define VMAREA_EXT_STACK_GUARD              0x10

//Status format: 

#define _WSTATUS(x)                         ((x) & 0177)
#define _WSTOPPED                           0177
#define _WCONTINUED                         0177777
#define WIFSTOPPED(x)                       (((x) & 0xff) == _WSTOPPED)
#define WSTOPSIG(x)                         (int)(((unsigned)(x) >> 8) & 0xff)
#define WIFSIGNALED(x)                      (_WSTATUS(x) != _WSTOPPED && _WSTATUS(x) != 0)
#define WTERMSIG(x)                         (_WSTATUS(x))
#define WIFEXITED(x)                        (_WSTATUS(x) == 0)
#define WEXITSTATUS(x)                      (int)(((unsigned)(x) >> 8) & 0xff)
#define WIFCONTINUED(x)                     (((x) & _WCONTINUED) == _WCONTINUED)
#define WCOREFLAG                           0200
#define WCOREDUMP(x)                        ((x) & WCOREFLAG)
#define W_EXITCODE(ret, sig)                ((ret) << 8 | (sig))
#define W_STOPCODE(sig)                     ((sig) << 8 | _WSTOPPED)

#define WREASON_EXIT                        0x01
#define WREASON_STOP                        0x02
#define WREASON_CONT                        0x04
#define WREASON_SIGNAL                      0x08

#define WNOHANG                             0x01
#define WUNTRACED                           0x02
#define WCONTINUED                          0x08
#define WEXITED                             0x04
#define WSTOPPED                            WUNTRACED
#define WNOWAIT                             0x10
#define WTRAPPED                            0x20

#define WAIT_ANY                            (-1)
#define WAIT_MYGRP                          0

typedef int thread_status_t;
struct process;
typedef struct process process_t;

typedef struct context {
    cpu_context_t *cpu_context;
    uint8_t *fxsave_region __attribute__((aligned(16)));
    uint64_t fs_base;
    uint64_t gs_base;
} context_t;

typedef struct thread {
    process_t *process;
    context_t *user_context;
    context_t *kernel_context;
    uint64_t kcontext_ready;

    void * ustack;
    void * ustack_base;
    uint64_t ustack_size;
    uint64_t ustack_guard_size;
    void * kstack;
    void * kstack_base;
    uint64_t kstack_size;
    uint64_t kstack_guard_size;
    uint64_t last_syscall_result;

    void * altstack;
    void * altstack_base;
    uint64_t altstack_size;
    uint64_t altstack_guard_size;
    void * altstack_saved_stack;
    void * altstack_saved_base;
    context_t *signal_context;
    int altstack_flags;

    sigset_t sigprocmask;
    sigset_t sigsuspend_mask;
    int unsuspend_signal;
    int id;
    uint8_t core_id;
    void* entry;
    thread_status_t status;
    int waiting;
    int *waitpid_status_address;
    int waitpid_status;
    int waitpid_pid;
    uint8_t syscall_ready; //A thread has to have called sycall_entry to return from a syscall
} thread_t;

typedef struct process {
    struct page_directory * vmm;
    struct vm_area *vm_areas;
    vdso_t * vdso;

    thread_t threads[MAX_THREADS];
    int thread_count;
    int current_thread;
    int main_thread;

    void * heap_base;
    void * heap_end;
    void * heap_max_size;

    long nice;
    long current_nice;
    int exit_code;
    int global_status;

    unsigned long long sleep_time;
    unsigned long long cpu_time;
    unsigned long long last_scheduled;

    struct task_signal *signal_queue[NSIG];
    struct sigaction signal_handlers[NSIG];
    struct vfs_struct *fs;
    unsigned int locks;

    int16_t pid;
    int16_t ppid;
    int16_t uid;
    int16_t gid;

    int open_files[MAX_OPEN_FILES];
    int open_files_count;

    int regular_tty;
    int io_tty;
    int *ctty;

    char ** argv;
    char ** envp;
    struct auxv * auxv;
    uint64_t auxv_size;

    void * entry_address;

    struct process *parent;

} process_t;

void returnoexit();

void lock_scheduler(void);
void unlock_scheduler(void);

uint8_t kcontext_ready(thread_t * thread);
void init_process(const char * init_path, const char * idle_path, char * tty);
void chroot(process_t * task, const char * path);
char * getcwd(process_t * task);
void chdir(process_t * task, const char * path);
process_t * get_current_process();
thread_t * get_current_thread();
void create_signal_context(thread_t * thread, int signo, struct sigaction * sigact, cpu_context_t * ctx);
void restore_signal_context(thread_t * thread, cpu_context_t * ctx);
void * get_signal_trampoline(process_t * task);
struct sigaction * select_signal(thread_t * thread, int * signo);
process_t * sched();
int16_t waitpid(thread_t * thread, int pid, int * status, int options);
int16_t fork(thread_t *thread);
void execve(process_t *task, const char * path, const char ** argv, const char ** envp);
int exec(process_t *task,char const *path, const char ** argv, const char ** envp);
void thread_exit(thread_t * thread);
void exit(process_t *task, int error_code);
void sync_files(thread_t *thread, struct vm_area * vma, uint64_t size);
process_t *get_process_by_pid(int pid);
void * get_vdso_base();
int get_open_file(process_t * task, int fd);
void add_open_file(process_t * task, int fd);
void remove_open_file(process_t * task, int fd);
#endif