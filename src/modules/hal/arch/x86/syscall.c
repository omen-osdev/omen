#include <omen/hal/arch/x86/syscall.h>
#include <omen/hal/arch/x86/msr.h>
#include <omen/hal/arch/x86/cpu.h>
#include <omen/managers/cpu/process.h>
#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <vfs/vfs.h>
#include <vfs/vfs_interface.h>

#define SYSRET(ctx, val) ctx->rax = val; return;
#define SYSCALL_ARG0(ctx) ctx->rdi
#define SYSCALL_ARG1(ctx) ctx->rsi
#define SYSCALL_ARG2(ctx) ctx->rdx
#define SYSCALL_ARG3(ctx) ctx->rcx

extern void syscall_entry();
uint64_t dummy_syscall_handler(process_t*task, cpu_context_t* ctx) {
    (void)task;
    kprintf("[PID: %d] DUMMY_SYSCALL(%d)\n", task->pid, ctx->rax);
    return SYSCALL_SUCCESS;
}

uint64_t read_syscall_handler(process_t*task, cpu_context_t* ctx) {
    (void)task;
    uint64_t fd = SYSCALL_ARG0(ctx);
    uint64_t buffer = SYSCALL_ARG1(ctx);
    uint64_t size = SYSCALL_ARG2(ctx);
    (void)fd;
    (void)buffer;
    (void)size;
    kprintf("[PID: %d] READ_SYSCALL(%d,%d,%d)\n", task->pid, fd, buffer, size);
    return vfs_file_read(fd, (void*)buffer, size);
}

uint64_t write_syscall_handler(process_t*task, cpu_context_t* ctx) {
    (void)task;
    uint64_t fd = SYSCALL_ARG0(ctx);
    uint64_t buffer = SYSCALL_ARG1(ctx);
    uint64_t size = SYSCALL_ARG2(ctx);
    (void)fd;
    (void)buffer;
    (void)size;
    kprintf("[PID: %d] WRITE_SYSCALL(%d,%d,%d)\n", task->pid, fd, buffer, size);
    return vfs_file_write(fd, (void*)buffer, size);
}

uint64_t open_syscall_handler(process_t*task, cpu_context_t* ctx) {
    (void)task;
    char* path = SYSCALL_ARG0(ctx);
    int flags = SYSCALL_ARG1(ctx);
    int mode = SYSCALL_ARG2(ctx);
    kprintf("[PID: %d] OPEN_SYSCALL(%s,%d,%d)\n", task->pid, path, flags, mode);
    int fd = vfs_file_open(path, flags, mode);
    if (fd < 0) {
        return SYSCALL_ERROR;
    }
    
    task->open_files[task->open_files_count++] = fd;
    return fd;
}

uint64_t close_syscall_handler(process_t*task, cpu_context_t* ctx) {
    (void)task;
    int fd = SYSCALL_ARG0(ctx);
    (void)fd;
    kprintf("[PID: %d] CLOSE_SYSCALL(%d)\n", task->pid, fd);
    
    for (int i = 0; i < task->open_files_count; i++) {
        if (task->open_files[i] == fd) {
            vfs_file_close(fd);
            task->open_files[i] = task->open_files[task->open_files_count - 1];
            task->open_files_count--;
            return SYSCALL_SUCCESS;
        }
    }
    return SYSCALL_ERROR;
}

uint64_t stat_syscall_handler(process_t*task, cpu_context_t* ctx) {
    (void)task;
    uint64_t path = SYSCALL_ARG0(ctx);
    uint64_t stat = SYSCALL_ARG1(ctx);
    (void)path;
    (void)stat;
    kprintf("[PID: %d] STAT_SYSCALL(%d,%d)\n", task->pid, path, stat);
    panic("Not implemented\n");
    return SYSCALL_SUCCESS;
}

uint64_t ioctl_syscall_handler(process_t*task, cpu_context_t* ctx) {
    (void)task;
    uint64_t fd = SYSCALL_ARG0(ctx);
    uint64_t request = SYSCALL_ARG1(ctx);
    uint64_t arg = SYSCALL_ARG2(ctx);
    (void)fd;
    (void)request;
    (void)arg;
    kprintf("[PID: %d] IOCTL_SYSCALL(%d,%d,%d)\n", task->pid, fd, request, arg);
    panic("Not implemented\n");
    return SYSCALL_SUCCESS;
}

uint64_t sched_yield_syscall_handler(process_t*task, cpu_context_t* ctx) {
    (void)task;
    (void)ctx;
    kprintf("[PID: %d] SCHED_YIELD_SYSCALL()\n", task->pid);
    sched();
    return SYSCALL_SUCCESS;
}

uint64_t fork_syscall_handler(process_t*task, cpu_context_t* ctx) {
    (void)task;
    (void)ctx;
    kprintf("[PID: %d] FORK_SYSCALL()\n", task->pid);
    uint64_t child_pid = (uint64_t)fork();
    kprintf("Child PID: %d\n", child_pid);
    return child_pid;
}

uint64_t execve_syscall_handler(process_t*task, cpu_context_t* ctx) {
    (void)task;
    (void)ctx;
    const char * path = (const char *)SYSCALL_ARG0(ctx);
    const char * argv = (const char *)SYSCALL_ARG1(ctx);
    const char * envp = (const char *)SYSCALL_ARG2(ctx);
    kprintf("[PID: %d] EXECVE_SYSCALL(%s,%s,%s)\n", task->pid, path, argv, envp);
    execve(path, argv, envp);
    return SYSCALL_SUCCESS;
}

uint64_t exit_syscall_handler(process_t*task, cpu_context_t* ctx) {
    (void)task;
    (void)ctx;
    int error_code = SYSCALL_ARG0(ctx);
    kprintf("[PID: %d] EXIT_SYSCALL(%d)\n", task->pid, error_code);
    exit(error_code);
    return SYSCALL_SUCCESS;
}

uint64_t undefined_syscall_handler(process_t*task, cpu_context_t* ctx) {
    (void)task;
    kprintf("[PID: %d] UNDEFINED_SYSCALL(%d)\n", task->pid, ctx->rax);
    return SYSCALL_UNDEFINED;
}

syscall_handler syscall_handlers[SYSCALL_HANDLER_COUNT] = {
    [0] = read_syscall_handler,
    [1] = write_syscall_handler,
    [2] = open_syscall_handler,
    [3] = close_syscall_handler,
    [4] = stat_syscall_handler,
    [5 ... 15] = undefined_syscall_handler,
    [16] = ioctl_syscall_handler,
    [17 ... 23] = undefined_syscall_handler,
    [24] = sched_yield_syscall_handler,
    [25 ... 56] = undefined_syscall_handler,
    [57] = fork_syscall_handler,
    [58] = undefined_syscall_handler,
    [59] = execve_syscall_handler,
    [60] = exit_syscall_handler,
    [61 ... 255] = undefined_syscall_handler
};

void global_syscall_handler(cpu_context_t* ctx) {

    process_t * current_task = get_current_process();
    memcpy(current_task->context, ctx, sizeof(cpu_context_t));
    __asm__("fxsave %0" : : "m" (current_task->fxsave_region));
    current_task->context->cr3 = to_identity_map(current_task->context->cr3);

    uint64_t result = SYSCALL_SUCCESS;

    if (ctx->rax < SYSCALL_HANDLER_COUNT) {
        result = syscall_handlers[ctx->rax](current_task, ctx);
    } else {
        kprintf("Syscall number overflow %d\n", ctx->rax);
        result = SYSCALL_ERROR;
    }

    current_task = get_current_process();
    current_task->context->cr3 = from_identity_map(current_task->context->cr3);
    __asm__("fxrstor %0" : "=m" (current_task->fxsave_region));
    memcpy(ctx, current_task->context, sizeof(cpu_context_t));

    struct tss * tss = arch_get_cpu(current_task->core_id)->tss;
    tss_set_stack(tss, ctx->info->kstack, 0);
    tss_set_stack(tss, ctx->rsp, 3);
    SYSRET(ctx, result);
}