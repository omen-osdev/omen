#include <omen/hal/arch/x86/syscall.h>
#include <omen/hal/arch/x86/msr.h>
#include <omen/hal/arch/x86/cpu.h>
#include <omen/managers/cpu/process.h>
#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>

#define SYSRET(ctx, val) ctx->rax = val; return;
#define SYSCALL_ARG0(ctx) ctx->rdi
#define SYSCALL_ARG1(ctx) ctx->rsi
#define SYSCALL_ARG2(ctx) ctx->rdx
#define SYSCALL_ARG3(ctx) ctx->rcx

extern void syscall_entry();
uint64_t dummy_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    kprintf("[PID: %d] DUMMY_SYSCALL(%d)\n", task->pid, ctx->rax);
    return SYSCALL_SUCCESS;
}

uint64_t read_syscall_handler(process_t*task, context_t* ctx) {
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
    return SYSCALL_SUCCESS;
}

uint64_t write_syscall_handler(process_t*task, context_t* ctx) {
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
            if (((char*)buffer)[i] == 0) break;
            kprintf("%c", ((char*)buffer)[i]);
        }
    }
    return SYSCALL_SUCCESS;
}

uint64_t open_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    uint64_t path = SYSCALL_ARG0(ctx);
    uint64_t flags = SYSCALL_ARG1(ctx);
    (void)path;
    (void)flags;
    kprintf("[PID: %d] OPEN_SYSCALL(%d,%d)\n", task->pid, path, flags);
    return SYSCALL_SUCCESS;
}

uint64_t close_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    uint64_t fd = SYSCALL_ARG0(ctx);
    (void)fd;
    kprintf("[PID: %d] CLOSE_SYSCALL(%d)\n", task->pid, fd);
    return SYSCALL_SUCCESS;
}

uint64_t stat_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    uint64_t path = SYSCALL_ARG0(ctx);
    uint64_t stat = SYSCALL_ARG1(ctx);
    (void)path;
    (void)stat;
    kprintf("[PID: %d] STAT_SYSCALL(%d,%d)\n", task->pid, path, stat);
    return SYSCALL_SUCCESS;
}

uint64_t ioctl_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    uint64_t fd = SYSCALL_ARG0(ctx);
    uint64_t request = SYSCALL_ARG1(ctx);
    uint64_t arg = SYSCALL_ARG2(ctx);
    (void)fd;
    (void)request;
    (void)arg;
    kprintf("[PID: %d] IOCTL_SYSCALL(%d,%d,%d)\n", task->pid, fd, request, arg);
    return SYSCALL_SUCCESS;
}

uint64_t sched_yield_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    (void)ctx;
    kprintf("[PID: %d] SCHED_YIELD_SYSCALL()\n", task->pid);
    sched();
    return SYSCALL_SUCCESS;
}

uint64_t fork_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    (void)ctx;
    kprintf("[PID: %d] FORK_SYSCALL()\n", task->pid);
    uint64_t child_pid = (uint64_t)fork();
    kprintf("Child PID: %d\n", child_pid);
    return child_pid;
}

uint64_t execve_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    (void)ctx;
    const char * path = (const char *)SYSCALL_ARG0(ctx);
    const char * argv = (const char *)SYSCALL_ARG1(ctx);
    const char * envp = (const char *)SYSCALL_ARG2(ctx);
    kprintf("[PID: %d] EXECVE_SYSCALL(%s,%s,%s)\n", task->pid, path, argv, envp);
    execve(path, argv, envp);
    return SYSCALL_SUCCESS;
}

uint64_t exit_syscall_handler(process_t*task, context_t* ctx) {
    (void)task;
    (void)ctx;
    int error_code = SYSCALL_ARG0(ctx);
    kprintf("[PID: %d] EXIT_SYSCALL(%d)\n", task->pid, error_code);
    exit(error_code);
    return SYSCALL_SUCCESS;
}

uint64_t undefined_syscall_handler(process_t*task, context_t* ctx) {
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

void global_syscall_handler(context_t* ctx) {

    process_t * current_task = get_current_process();
    ctx->cr3 = get_pml4();

    //struct page_directory * father_cr3 = ctx->cr3;

    current_task->cpu->ustack = ctx->rsp;

    memcpy(current_task->context, ctx, sizeof(context_t));
    __asm__("fxsave %0" : : "m" (current_task->fxsave_region));
    
    uint64_t result = SYSCALL_SUCCESS;

    if (ctx->rax < SYSCALL_HANDLER_COUNT) {
        result = syscall_handlers[ctx->rax](current_task, ctx);
    } else {
        kprintf("Syscall number overflow %d\n", ctx->rax);
        result = SYSCALL_ERROR;
    }

    current_task = get_current_process();
    __asm__("fxrstor %0" : "=m" (current_task->fxsave_region));
    memcpy(ctx, current_task->context, sizeof(context_t));

    tss_set_stack(current_task->cpu->tss, current_task->cpu->cinfo->stack, 0);
    tss_set_stack(current_task->cpu->tss, current_task->ustack, 3);
    current_task->cpu->ustack = current_task->ustack;
    ctx->cr3 = get_physical_address(current_task->context->cr3, current_task->context->cr3);

    //if ((uint64_t)father_cr3 != current_task->context->cr3) {
    //    kprintf("Switch detected, performing test\n");
    //    uint64_t * address = ctx->rsp;
    //    switch_cr3(get_physical_address(father_cr3, father_cr3));
    //    *address = 0x42;
    //    kprintf("Value for address V:%llx P:%llx with cr3: %llx: %llx\n", address, get_physical_address(father_cr3, address), father_cr3, *address);
    //    switch_cr3(ctx->cr3);
    //    *address = 0x55;
    //    kprintf("Value for address V:%llx P:%llx with cr3: %llx: %llx\n", address, get_physical_address(current_task->context->cr3, address), ctx->cr3, *address);
    //    switch_cr3(get_physical_address(father_cr3, father_cr3));
    //    kprintf("Value for address V:%llx P:%llx with cr3: %llx: %llx\n", address, get_physical_address(father_cr3, address), father_cr3, *address);
    //}

    //kprintf("CURRENT PROCESS USTACK %llx USTACK_BASE %llx KSTACK(CPU) %llx\n", current_task->ustack, current_task->ustack_base, current_task->cpu->cinfo->stack);
    //kprintf("DEBUG: VMM STACK: %llx OLD_TRANSLATES: %llx NEW_TRANSLATES: %llx\n", ctx->rsp, get_physical_address(father_cr3, ctx->rsp), get_physical_address(current_task->context->cr3, ctx->rsp));
    //kprintf("Returning to process %d with stack %llx\n", current_task->pid, ctx->rsp);

    SYSRET(ctx, result);
}