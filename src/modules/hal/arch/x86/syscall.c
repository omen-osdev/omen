#include <omen/hal/arch/x86/syscall.h>
#include <omen/hal/arch/x86/msr.h>
#include <omen/hal/arch/x86/cpu.h>
#include <omen/managers/cpu/process.h>
#include <omen/managers/cpu/vmarea.h>
#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <vfs/vfs.h>
#include <vfs/vfs_interface.h>
#include <errno.h>
/*


mmap
munmap

exit
fork
waitpid
exec

mount
write, read, open, close okey
opendir, readdir, closedir, chdir, getcwd, mkdir

stat, lseek, getfsstat

dup, pipe


kot's

#define SYS_LOG                 0
#define SYS_ARCH_PRCTL          1
#define SYS_GET_TID             2
#define SYS_FUTEX_WAIT          3
#define SYS_FUTEX_WAKE          4
#define SYS_MMAP                5
#define SYS_MUNMAP              6
#define SYS_MPROTECT            7
#define SYS_EXIT                8
#define SYS_THREAD_EXIT         9
#define SYS_CLOCK_GET           10
#define SYS_CLOCK_GETRES        11
#define SYS_SLEEP               12
#define SYS_SIGPROCMASK         13
#define SYS_SIGACTION           14
#define SYS_SIGRESTORE          15
#define SYS_FORK                16
#define SYS_WAITPID             17
#define SYS_EXECVE              18
#define SYS_GETPID              19
#define SYS_GETPPID             20
#define SYS_KILL                21
#define SYS_FILE_OPEN           22
#define SYS_FILE_READ           23
#define SYS_FILE_WRITE          24
#define SYS_FILE_SEEK           25
#define SYS_FILE_CLOSE          26
#define SYS_FILE_IOCTL          27
#define SYS_DIR_READ_ENTRIES    28
#define SYS_DIR_REMOVE          29
#define SYS_DIR_CREATE          30
#define SYS_UNLINK_AT           31
#define SYS_RENAME_AT           32
#define SYS_PATH_STAT           33
#define SYS_FD_STAT             34
#define SYS_FCNTL               35
#define SYS_GETCWD              36
#define SYS_CHDIR               37
#define SYS_SOCKET              38
#define SYS_BIND                39
#define SYS_CONNECT             40
#define SYS_LISTEN              41
#define SYS_ACCEPT              42
#define SYS_SOCKET_SEND         43
#define SYS_SOCKET_RECV         44
#define SYS_SOCKET_PAIR         45
#define SYS_PPOLL               46
#define SYS_SELECT              47

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
    kprintf("[PID: %d | TID %d] DUMMY_SYSCALL(%d)\n", thread->process->pid, thread->id, ctx->rax);
    return SYSCALL_SUCCESS;
}

int64_t read_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    uint64_t fd = SYSCALL_ARG0(ctx);
    uint64_t buffer = SYSCALL_ARG1(ctx);
    uint64_t size = SYSCALL_ARG2(ctx);
    (void)fd;
    (void)buffer;
    (void)size;
    kprintf("[PID: %d | TID %d] READ_SYSCALL(%d,%d,%d)\n", thread->process->pid, thread->id, fd, buffer, size);
    return vfs_file_read(fd, (void*)buffer, size);
}

int64_t write_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    uint64_t fd = SYSCALL_ARG0(ctx);
    uint64_t buffer = SYSCALL_ARG1(ctx);
    uint64_t size = SYSCALL_ARG2(ctx);
    (void)fd;
    (void)buffer;
    (void)size;
    kprintf("[PID: %d | TID %d] WRITE_SYSCALL(%d,%d,%d)\n", thread->process->pid, thread->id, fd, buffer, size);
    return vfs_file_write(fd, (void*)buffer, size);
}

int64_t open_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    char* path = SYSCALL_ARG0(ctx);
    int flags = SYSCALL_ARG1(ctx);
    int mode = SYSCALL_ARG2(ctx);
    kprintf("[PID: %d | TID %d] OPEN_SYSCALL(%s,%d,%d)\n", thread->process->pid, thread->id, path, flags, mode);
    
    if (thread->process->open_files_count >= MAX_OPEN_FILES) {
        kprintf("Max open files reached\n");
        return SYSCALL_ERROR;
    }
    
    int fd = vfs_file_open(path, flags, mode);
    if (fd < 0) {
        return SYSCALL_ERROR;
    }

    thread->process->open_files[thread->process->open_files_count++] = fd;
    return fd;
}

int64_t close_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    int fd = SYSCALL_ARG0(ctx);
    kprintf("[PID: %d | TID %d] CLOSE_SYSCALL(%d)\n", thread->process->pid, thread->id, fd);
    
    int open_files_prev = thread->process->open_files_count;
    for (int i = 0; i < open_files_prev; i++) {
        if (thread->process->open_files[i] == fd) {
            vfs_file_close(fd);
            thread->process->open_files[i] = thread->process->open_files[thread->process->open_files_count - 1];
            thread->process->open_files_count--;
            return SYSCALL_SUCCESS;
        }
    }
    return SYSCALL_ERROR;
}

int64_t stat_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    char * path = SYSCALL_ARG0(ctx);
    stat_t* stat = SYSCALL_ARG1(ctx);
    kprintf("[PID: %d | TID %d] STAT_SYSCALL(%d,%d)\n", thread->process->pid, thread->id, path, stat);

    int fd = vfs_file_open((char*)path, O_RDONLY, 0);
    if (fd < 0) {
        return SYSCALL_ERROR;
    }
    int ret = vfs_file_stat(fd, stat);
    if (ret < 0) {
        return SYSCALL_ERROR;
    }
    vfs_file_close(fd);
    kprintf("File size: %d\n", stat->st_size);
    kprintf("File mode: %d\n", stat->st_mode);
    return SYSCALL_SUCCESS;
}

int64_t fstat_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    int fd = SYSCALL_ARG0(ctx);
    stat_t* stat = SYSCALL_ARG1(ctx);

    kprintf("[PID: %d | TID %d] FSTAT_SYSCALL(%d,%d)\n", thread->process->pid, thread->id, fd, stat);
    int ret = vfs_file_stat(fd, stat);
    if (ret < 0) {
        return SYSCALL_ERROR;
    }
    kprintf("File size: %d\n", stat->st_size);
    kprintf("File mode: %d\n", stat->st_mode);

    return SYSCALL_SUCCESS;
}

int64_t ioctl_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    uint64_t fd = SYSCALL_ARG0(ctx);
    uint64_t request = SYSCALL_ARG1(ctx);
    uint64_t arg = SYSCALL_ARG2(ctx);
    kprintf("[PID: %d | TID %d] IOCTL_SYSCALL(%d,%d,%d)\n", thread->process->pid, thread->id, fd, request, arg);
    int ret = vfs_file_ioctl(fd, request, arg);
    if (ret < 0) {
        return SYSCALL_ERROR;
    }

    kprintf("IOCTL request: %d\n", request);
    return SYSCALL_SUCCESS;
}

int64_t sched_yield_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    kprintf("[PID: %d | TID %d] SCHED_YIELD_SYSCALL()\n", thread->process->pid);
    kprintf("Yielding process %d\n", get_current_process()->pid);
    sched();
    kprintf("Resuming process %d\n", get_current_process()->pid);
    return SYSCALL_SUCCESS;
}

int64_t fork_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    kprintf("[PID: %d | TID %d] FORK_SYSCALL()\n", thread->process->pid);
    uint64_t child_pid = (uint64_t)fork(thread);
    kprintf("Child PID: %d | TID %d\n", child_pid);
    return child_pid;
}

int64_t execve_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    const char * path = (const char *)SYSCALL_ARG0(ctx);
    const char * argv = (const char *)SYSCALL_ARG1(ctx);
    const char * envp = (const char *)SYSCALL_ARG2(ctx);
    if (path == NULL) {
        kprintf("Invalid arguments for execve\n");
        return SYSCALL_ERROR;
    } else if (argv == NULL || envp == NULL) {
        kprintf("[PID: %d | TID %d] EXECVE_SYSCALL(%s,NULL,NULL)\n", thread->process->pid, thread->id, path);
    } else {
        kprintf("[PID: %d | TID %d] EXECVE_SYSCALL(%s,%s,%s)\n", thread->process->pid, thread->id, path, argv, envp);
    }
    execve(thread->process, path, argv, envp);
    return SYSCALL_SUCCESS;
}

#define ARCH_SET_CPUID 0x0
#define ARCH_GET_CPUID 0x1
#define ARCH_SET_FS 0x2
#define ARCH_GET_FS 0x3
#define ARCH_SET_GS 0x4
#define ARCH_GET_GS 0x5
int64_t prctl_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    int option = SYSCALL_ARG0(ctx);
    int arg2 = SYSCALL_ARG1(ctx);
    kprintf("[PID: %d | TID %d] PRCTL_SYSCALL(%d,%d)\n", thread->process->pid, thread->id, option, arg2);
    switch (option) {
        case ARCH_SET_CPUID:
            return -ENODEV;
        case ARCH_GET_CPUID:
            return -ENODEV;
        case ARCH_SET_FS:
            thread->context->fs_base = (uint64_t)arg2;
            break;
        case ARCH_GET_FS: {
            unsigned long * fs_base = (unsigned long *)(unsigned long)arg2;
            if (fs_base == NULL) {
                return -EINVAL;
            }

            *fs_base = (unsigned long)thread->context->fs_base;
            break;
        }
        case ARCH_SET_GS:
            thread->context->gs_base = (uint64_t)arg2;
            break;
        case ARCH_GET_GS: {
            unsigned long * gs_base = (unsigned long *)(unsigned long)arg2;
            if (gs_base == NULL) {
                return -EINVAL;
            }

            *gs_base = (unsigned long)thread->context->gs_base;
            break;
        }
        default:
            return -EINVAL;
    }
    return SYSCALL_SUCCESS;
}

int64_t exit_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    (void)ctx;
    int error_code = SYSCALL_ARG0(ctx);
    kprintf("[PID: %d | TID %d] EXIT_SYSCALL(%d)\n", thread->process->pid, thread->id, error_code);
    exit(thread->process, error_code);
    return SYSCALL_SUCCESS;
}

//void * mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
//Prots: 0x1 = PROT_READ, 0x2 = PROT_WRITE, 0x4 = PROT_EXEC
//Flags: 0x1 = MAP_SHARED, 0x2 = MAP_PRIVATE, 0x4 = MAP_ANONYMOUS
#define MAP_SHARED 0x1
#define MAP_PRIVATE 0x2
#define MAP_ANONYMOUS 0x4
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4
#define PROT_NONE 0x8
int64_t mmap_syscall_handler(thread_t*thread, cpu_context_t*ctx) {
    void * addr = (void *)SYSCALL_ARG0(ctx);
    size_t length = SYSCALL_ARG1(ctx);
    int prot = SYSCALL_ARG2(ctx);
    int flags = SYSCALL_ARG3(ctx);
    int fd = SYSCALL_ARG4(ctx);
    off_t offset = SYSCALL_ARG5(ctx);
    kprintf("[PID: %d | TID %d] MMAP_SYSCALL(%p,%d,%d,%d,%d,%d)\n", thread->process->pid, thread->id, addr, length, prot, flags, fd, offset);
    
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

    if (fd < 0 && fd != -1) {
        panic("Invalid fd\n");
        return SYSCALL_ERROR;
    }
    if (offset % PAGE_SIZE != 0) {
        panic("Offset is not page aligned\n");
        return SYSCALL_ERROR;
    }
    if (offset > 0 && fd == 0) {
        panic("Invalid fd\n");
        return SYSCALL_ERROR;
    }

    if (addr != NULL && ((uint64_t)addr % PAGE_SIZE != 0))
        addr = (void *)((uint64_t)addr & ~(PAGE_SIZE - 1));
    addr = find_shm_vmarea(thread->process, addr, length);
    if (addr == NULL) {
        panic("Failed to find a free area\n");
        return SYSCALL_ERROR;
    }
    kprintf("Found free area: %p\n", addr);

    if (flags & MAP_PRIVATE || flags & MAP_SHARED) {

        allocate_at_vaddr(thread->process->vmm, addr, length, vmm_flags);

        int newfd = -1;
        if (flags & MAP_ANONYMOUS) {
            create_vmarea(thread->process, addr, (addr + length), vmm_flags, vma_flags, PAGE_SIZE_4KIB, newfd, 0);
            return addr;
        } else {
            newfd = vfs_file_dup(fd, -1);
            if (newfd < 0) {
                goto cleanup_on_error;
            }
        }

        create_vmarea(thread->process, addr, (addr + length), vmm_flags, vma_flags, PAGE_SIZE_4KIB, newfd, offset);
        
        //add write privilege to the buffer
        mprotect(thread->process->vmm, addr, length, PROT_READ | PROT_WRITE);

        //Read the file into the memory
        if (vfs_file_seek(newfd, offset, SEEK_SET) < 0) {
            kprintf("Failed to seek file\n");
            vfs_file_close(newfd);
            goto cleanup_on_error;
        }

        int64_t bytes_read = vfs_file_read(newfd, addr, length);
        if (bytes_read < 0) {
            kprintf("Failed to read file\n");
            vfs_file_close(newfd);
            goto cleanup_on_error;
        } 

        //Reset permissions but keep readonly so it page faults on a write
        uint8_t roflags = vmm_flags & ~VMM_WRITE_BIT;
        mprotect(thread->process->vmm, addr, length, roflags);
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

    kprintf("[PID: %d | TID %d] MPROTECT_SYSCALL(%p,%d,%d)\n", thread->process->pid, thread->id, addr, length, prot);
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
    kprintf("[PID: %d | TID %d] MUNMAP_SYSCALL(%p,%d)\n", thread->process->pid, thread->id, addr, length);
    
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
    kprintf("[PID: %d | TID %d] MSYNC_SYSCALL(%p,%d,%d)\n", thread->process->pid, thread->id, addr, length, flags);

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

int64_t dup_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    int fd = SYSCALL_ARG0(ctx);
    kprintf("[PID: %d | TID %d] DUP_SYSCALL(%d)\n", thread->process->pid, thread->id, fd);
    if (fd < 0) {
        return SYSCALL_ERROR;
    }

    if (thread->process->open_files_count >= MAX_OPEN_FILES) {
        kprintf("Max open files reached\n");
        return SYSCALL_ERROR;
    }

    int newfd = vfs_file_dup(fd, -1);
    if (newfd < 0) {
        return SYSCALL_ERROR;
    }
    thread->process->open_files[thread->process->open_files_count++] = newfd;
    return newfd;
}

int64_t dup2_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    int oldfd = SYSCALL_ARG0(ctx);
    int newfd = SYSCALL_ARG1(ctx);
    kprintf("[PID: %d | TID %d] DUP2_SYSCALL(%d,%d)\n", thread->process->pid, thread->id, oldfd, newfd);
    if (oldfd < 0 || newfd < 0) {
        return SYSCALL_ERROR;
    }

    if (thread->process->open_files_count >= MAX_OPEN_FILES) {
        kprintf("Max open files reached\n");
        return SYSCALL_ERROR;
    }

    int ret = vfs_file_dup(oldfd, newfd);
    if (ret < 0) {
        return SYSCALL_ERROR;
    }
    thread->process->open_files[thread->process->open_files_count++] = newfd;
    return newfd;
}


int64_t undefined_syscall_handler(thread_t*thread, cpu_context_t* ctx) {
    kprintf("[PID: %d | TID %d] UNDEFINED_SYSCALL(%d)\n", thread->process->pid, thread->id, ctx->rax);
    return SYSCALL_UNDEFINED;
}

syscall_handler syscall_handlers[SYSCALL_HANDLER_COUNT] = {
    [0] = read_syscall_handler,
    [1] = write_syscall_handler,
    [2] = open_syscall_handler,
    [3] = close_syscall_handler,
    [4] = stat_syscall_handler,
    [5] = fstat_syscall_handler,
    [6 ... 8] = undefined_syscall_handler,
    [9] = mmap_syscall_handler,
    [10] = mprotect_syscall_handler,
    [11] = munmap_syscall_handler,
    [12 ... 15] = undefined_syscall_handler,
    [16] = ioctl_syscall_handler,
    [17 ... 23] = undefined_syscall_handler,
    [24] = sched_yield_syscall_handler,
    [25] = undefined_syscall_handler,
    [26] = msync_syscall_handler,
    [27 ... 31] = undefined_syscall_handler,
    [32] = dup_syscall_handler,
    [33] = dup2_syscall_handler,
    [34 ... 56] = undefined_syscall_handler,
    [57] = fork_syscall_handler,
    [58] = undefined_syscall_handler,
    [59] = execve_syscall_handler,
    [60] = exit_syscall_handler,
    [61 ... 255] = undefined_syscall_handler
};

void global_syscall_handler(cpu_context_t* ctx) {

    thread_t * current_thread = get_current_thread();
    current_thread->syscall_ready = 1;
    
    memcpy(current_thread->context->cpu_context, ctx, sizeof(cpu_context_t));
    memcpy(current_thread->context->cpu_context->info, ctx->info, sizeof(struct cpu_context_info));

    __asm__("fxsave %0" : : "m" (current_thread->context->fxsave_region));

    int64_t result = SYSCALL_SUCCESS;
    if (ctx->rax < SYSCALL_HANDLER_COUNT) {
        result = syscall_handlers[ctx->rax](current_thread, ctx);
    } else {
        kprintf("Syscall number overflow %d\n", ctx->rax);
        result = SYSCALL_ERROR;
    }

    current_thread = get_current_thread();

    __asm__("fxrstor %0" : "=m" (current_thread->context->fxsave_region));

    memcpy(ctx, current_thread->context->cpu_context, sizeof(cpu_context_t));
    memcpy(ctx->info, current_thread->context->cpu_context->info, sizeof(struct cpu_context_info));

    struct tss * tss = arch_get_cpu(current_thread->core_id)->tss;
    tss_set_stack(tss, ctx->info->kstack, 0);
    tss_set_stack(tss, ctx->rsp, 3);
    setFsBase(current_thread->context->fs_base);

    if (current_thread->syscall_ready) {
        SYSRET(ctx, result);
    } else {
    __asm__("mov %0, %%rsp\n"
            "mov %1, %%cr3\n"
            "ret\n" : : "r" (current_thread->ustack), "r" (current_thread->context->cpu_context->cr3));
    }
}