#include <omen/managers/cpu/context.h>
#include <omen/managers/cpu/process.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/hal/arch/x86/syscall.h>

extern void emulate_return();

void emulate_syscall_return() {
    thread_t * thread = get_current_thread();
    if (!thread) {
        panic("No current thread found in emulate_syscall_return\n");
    }

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