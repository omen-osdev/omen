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
void undefined_syscall_handler(process_t*task, cpu_context_t* ctx) {
    (void)task;
    kprintf("Undefined syscall %d\n", ctx->rax);
    SYSRET(ctx, SYSCALL_UNDEFINED);
}

void sched_yield_syscall_handler(process_t*task, cpu_context_t* ctx) {
    (void)task;
    (void)ctx;
    //Do nothing, we will yield at the beginning of the syscall handler
}

syscall_handler syscall_handlers[SYSCALL_HANDLER_COUNT] = {
    [0 ... 23] = undefined_syscall_handler,
    [24] = sched_yield_syscall_handler,
    [25 ... SYSCALL_HANDLER_COUNT-1] = undefined_syscall_handler
};

void global_syscall_handler(cpu_context_t* ctx) {

    //yield();

    process_t * current_task = get_current_process();

    if (ctx->rax < SYSCALL_HANDLER_COUNT) {
        syscall_handlers[ctx->rax](current_task, ctx);
    } else {
        kprintf("Syscall number overflow %d\n", ctx->rax);
        SYSRET(ctx, SYSCALL_ERROR);
    }
}

void syscall_set_user_gs(uintptr_t addr)
{
    cpu_set_msr_u64(MSR_GS_BASE, addr);
}

void syscall_set_kernel_gs(uintptr_t addr)
{
    cpu_set_msr_u64(MSR_KERN_GS_BASE, addr);
}