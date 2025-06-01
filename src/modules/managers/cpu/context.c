#include <omen/managers/cpu/context.h>
#include <omen/managers/cpu/process.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/hal/arch/x86/syscall.h>

extern void load_context(cpu_context_t * ctx);
extern void save_context(cpu_context_t * ctx);
extern void setFsBase(uint64_t base);

void save_kcontext() {
    thread_t * thread = get_current_thread();
    if (!thread) {
        panic("No current thread found in save_kcontext \n");
    }

    if (thread->kcontext_ready) panic("Kernel context already ready in save_kcontext\n");
    cpu_context_t * ctx = thread->kernel_context->cpu_context;
    if (!ctx) {
        panic("Kernel context CPU context is NULL in save_kcontext\n");
    }

    thread->kcontext_ready = 1; // Set kcontext_ready flag
    save_context(ctx);
 }

void load_kcontext() {
    thread_t * thread = get_current_thread();
    if (!thread) {
        panic("No current thread found in load_kcontext \n");
    }

    if (thread->kcontext_ready) {
        // If the kernel context is ready, we can safely return to user mode
        // by restoring the user context.
        cpu_context_t * ctx = thread->kernel_context->cpu_context;
        struct tss * tss = arch_get_cpu(thread->core_id)->tss;
        tss_set_stack(tss, thread->kernel_context->cpu_context->info->kstack, 0);
        tss_set_stack(tss, thread->kernel_context->cpu_context->rsp, 3);
        setFsBase(thread->kernel_context->fs_base);
        load_context(ctx);
        thread->kcontext_ready = 0; // Reset kcontext_ready flag
    } else {
        struct tss * tss = arch_get_cpu(thread->core_id)->tss;
        tss_set_stack(tss, thread->user_context->cpu_context->info->kstack, 0);
        tss_set_stack(tss, thread->user_context->cpu_context->rsp, 3);
        setFsBase(thread->user_context->fs_base);
        __asm__ volatile("mov %0, %%rsp\n"           // Load new_rsp into RAX
            "pop %%rax\n"
            "mov %%rax, %%cr3\n"
            "popq %%gs:0x8\n"
            "pop %%rax\n"
            "pop %%rbx\n"
            "pop %%rcx\n"
            "pop %%rdx\n"
            "pop %%rsi\n"
            "pop %%rdi\n"
            "pop %%rbp\n"
            "pop %%r8\n"
            "pop %%r9\n"
            "pop %%r10\n"
            "pop %%r11\n"
            "pop %%r12\n"
            "pop %%r13\n"
            "pop %%r14\n"
            "pop %%r15\n"
            "mov 0x20(%%rsp), %%r11\n"
            "mov 0x10(%%rsp), %%rcx\n"
            "mov 0x28(%%rsp), %%rsp\n"
            "swapgs\n"
            "sysretq\n"
            :
            : "r"(thread->user_context->cpu_context)
        );
    }
}