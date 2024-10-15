#include <omen/hal/arch/x86/syscall.h>
#include <omen/hal/arch/x86/msr.h>
#include <omen/hal/arch/x86/cpu.h>
#include <omen/managers/cpu/process.h>
#include <omen/apps/debug/debug.h>

#define SYSRET(ctx, val) ctx->rax = val; return;
#define SYSCALL_ARG0(ctx) ctx->rdi
#define SYSCALL_ARG1(ctx) ctx->rsi
#define SYSCALL_ARG2(ctx) ctx->rdx
#define SYSCALL_ARG3(ctx) ctx->rcx

extern void syscall_entry();
void dummy_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    kprintf("[PID: %d] DUMMY_SYSCALL(%d)\n", task->pid, ctx->rax);
    SYSRET(ctx, SYSCALL_SUCCESS);
}

void read_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    uint64_t fd = SYSCALL_ARG0(ctx);
    uint64_t buffer = SYSCALL_ARG1(ctx);
    uint64_t size = SYSCALL_ARG2(ctx);
    (void)fd;
    (void)buffer;
    (void)size;
    kprintf("[PID: %d] READ_SYSCALL(%d,%d,%d)\n", task->pid, fd, buffer, size);
    if (fd == 0) {
        for (uint64_t i = 0; i < size; i++) {
            ((char*)buffer)[i] = 'a';
        }
    }
    SYSRET(ctx, SYSCALL_SUCCESS);
}

void write_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    uint64_t fd = SYSCALL_ARG0(ctx);
    uint64_t buffer = SYSCALL_ARG1(ctx);
    uint64_t size = SYSCALL_ARG2(ctx);
    (void)fd;
    (void)buffer;
    (void)size;
    kprintf("[PID: %d] WRITE_SYSCALL(%d,%d,%d)\n", task->pid, fd, buffer, size);
    if (fd == 1) {
        for (uint64_t i = 0; i < size; i++) {
            kprintf("%c", ((char*)buffer)[i]);
        }
    }
    SYSRET(ctx, SYSCALL_SUCCESS);
}

void open_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    uint64_t path = SYSCALL_ARG0(ctx);
    uint64_t flags = SYSCALL_ARG1(ctx);
    (void)path;
    (void)flags;
    kprintf("[PID: %d] OPEN_SYSCALL(%d,%d)\n", task->pid, path, flags);
    SYSRET(ctx, SYSCALL_SUCCESS);
}

void close_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    uint64_t fd = SYSCALL_ARG0(ctx);
    (void)fd;
    kprintf("[PID: %d] CLOSE_SYSCALL(%d)\n", task->pid, fd);
    SYSRET(ctx, SYSCALL_SUCCESS);
}

void stat_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    uint64_t path = SYSCALL_ARG0(ctx);
    uint64_t stat = SYSCALL_ARG1(ctx);
    (void)path;
    (void)stat;
    kprintf("[PID: %d] STAT_SYSCALL(%d,%d)\n", task->pid, path, stat);
    SYSRET(ctx, SYSCALL_SUCCESS);
}

void ioctl_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    uint64_t fd = SYSCALL_ARG0(ctx);
    uint64_t request = SYSCALL_ARG1(ctx);
    uint64_t arg = SYSCALL_ARG2(ctx);
    (void)fd;
    (void)request;
    (void)arg;
    kprintf("[PID: %d] IOCTL_SYSCALL(%d,%d,%d)\n", task->pid, fd, request, arg);
    SYSRET(ctx, SYSCALL_SUCCESS);
}

void sched_yield_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    (void)ctx;
    kprintf("[PID: %d] SCHED_YIELD_SYSCALL()\n", task->pid);
    yield();
    SYSRET(ctx, SYSCALL_SUCCESS);
}

void fork_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    (void)ctx;
    kprintf("[PID: %d] FORK_SYSCALL()\n", task->pid);
    uint64_t child_pid = (uint64_t)fork();
    kprintf("Child PID: %d\n", child_pid);
    SYSRET(ctx, child_pid);
}

void execve_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    (void)ctx;
    const char * path = (const char *)SYSCALL_ARG0(ctx);
    const char * argv = (const char *)SYSCALL_ARG1(ctx);
    const char * envp = (const char *)SYSCALL_ARG2(ctx);
    kprintf("[PID: %d] EXECVE_SYSCALL(%s,%s,%s)\n", task->pid, path, argv, envp);
    execve(path, argv, envp);
    SYSRET(ctx, SYSCALL_SUCCESS);
}

void exit_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    (void)ctx;
    int error_code = SYSCALL_ARG0(ctx);
    kprintf("[PID: %d] EXIT_SYSCALL(%d)\n", task->pid, error_code);
    exit(error_code);
    SYSRET(ctx, SYSCALL_SUCCESS);
}

void undefined_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    kprintf("[PID: %d] UNDEFINED_SYSCALL(%d)\n", task->pid, ctx->rax);
    SYSRET(ctx, SYSCALL_UNDEFINED);
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

void global_syscall_handler(context_t* ctx) {

    process_t * current_task = get_current_process();

    if (ctx->rax < SYSCALL_HANDLER_COUNT) {
        syscall_handlers[ctx->rax](current_task, ctx);
    } else {
        kprintf("Syscall number overflow %d\n", ctx->rax);
        SYSRET(ctx, SYSCALL_ERROR);
    }
}