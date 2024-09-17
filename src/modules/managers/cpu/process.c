#include <generic/config.h>
#include <omen/apps/panic/panic.h>
#include <omen/managers/cpu/process.h>
#include <omen/managers/mem/vmm.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/std/stddef.h>
#include <omen/hal/arch/x86/cpu.h>
#include <omen/hal/arch/x86/gdt.h>
#include <omen/hal/arch/x86/msr.h>
#include <omen/hal/arch/x86/syscall.h>
//TODO: Delete this, we need an elf loader
#include <dummy/dummy.h>

extern void newctxswtch(cpu_context_t * old_task, cpu_context_t * new_task, void* fxsave, void* fxrstor);
extern void newctxcreat(void* rsp, void* intro);
extern void newuctxcreat(void* rsp, void* intro);
extern void reloadGsFs();
extern void setGsBase(uint64_t base);

process_t current_process;

void init_user_context(process_t * task, void * init) {
    cpu_context_t * context = task->context;

    void * stack_top = kmalloc(PROCESS_STACK_SIZE);
    memset(stack_top, 0, PROCESS_STACK_SIZE);
    void * stack = stack_top + PROCESS_STACK_SIZE;
    //TODO: Initialize the stack
    newuctxcreat(&stack, init);
    
    context->cr3  = (uint64_t) task->vm;
    context->info = kmalloc(sizeof(struct cpu_context_info));
    memset(context->info, 0, sizeof(struct cpu_context_info));
    context->info->stack = (uint64_t) stack;
    context->info->cs = get_user_code_selector();
    context->info->ss = get_user_data_selector();
    context->info->thread = 0;
    context->rax = 1;
    context->rbx = 2;
    context->rcx = 3;
    context->rdx = 4;
    context->rsi = 5;
    context->rdi = 6;
    context->rbp = 7;
    context->r8 = 8;
    context->r9 = 9;
    context->r10 = 10;
    context->r11 = 11;
    context->r12 = 12;
    context->r13 = 13;
    context->r14 = 14;
    context->r15 = 15;
    context->interrupt_number = 0;
    context->error_code = 0;
    context->rip = (uint64_t)init;
    context->rflags = PROCESS_STARTUP_RFLAGS;
    context->cs = get_user_code_selector();
    context->ss = get_user_data_selector();
    context->rsp = (uint64_t)stack;
    
    task->cpu->ustack = task->context->rsp;

    __asm__ volatile("fxsave %0" : "=m" (task->fxsave_region));

}

process_t * create_user_process(void * init) {
    process_t * task = (process_t *) kmalloc(sizeof(process_t));
    memset(task, 0, sizeof(process_t));
    task->status = PROCESS_STATUS_READY;
    task->signal_pending = 0;
    task->nice = 0;
    task->privilege = 0;
    task->heap = 0;
    task->cpu = arch_get_bsp_cpu(); //TODO: Change this for SMP
    task->cpu_time = 0;
    task->last_scheduled = 0;
    task->sleep_time = 0;
    task->exit_code = 0;    
    task->exit_signal = 0;
    task->pdeath_signal = 0;
    task->pid = 0; //TODO: Implement PID
    task->locks = 0;
    task->open_files = 0;
    task->entry_address = init;
    task->tty = 0;
    task->descriptors = 0;
    task->parent = &current_process;
    
    if (task->parent) {
        task->uid = task->parent->uid;
        task->gid = task->parent->gid;
        task->ppid = task->parent->pid;
    } else {
        task->uid = 0;
        task->gid = 0;
        task->ppid = 0;
    }

    task->vm = kmalloc(sizeof(struct page_directory));
    memset(task->vm, 0, sizeof(struct page_directory));
    duplicate_current_pml4(task->vm);
    task->context = kmalloc(sizeof(cpu_context_t)); 
    memset(task->context, 0, sizeof(cpu_context_t));
    init_user_context(task, init);

    return task;
}

void returnoexit() {
    panic("Returned from a process!\n");
}

//TODO: Implement a real yield
void yield(process_t * next) {
    __asm__("cli");
    process_t * prev = &current_process;

    if (next == &current_process) {
        panic("Trying to yield to the same process!\n");
    }

    syscall_set_user_gs((uint64_t)next->context);
    syscall_set_kernel_gs((uint64_t)next->cpu->ctx);
    tss_set_stack(next->cpu->tss, next->cpu->ustack, 3);
    newctxswtch(prev->context, next->context, prev->fxsave_region, next->fxsave_region);
}

process_t * get_current_process() {
    return &current_process;
}

void init_process() {
    //This is a debug processs
    current_process = (process_t) {
        .context = kmalloc(sizeof(cpu_context_t)),
        .cpu = NULL,
        .vm = NULL,
        .status = 0,
        .privilege = 0,
        .signal_pending = 0,
        //TODO: Implement signal queue
        //.signal_queue = NULL,
        //.signal_handlers = {0},
        .fxsave_region = {0},
        .nice = 0,
        .current_nice = 0,
        //.frame = NULL,
        .heap = NULL,
        .sleep_time = 0,
        .cpu_time = 0,
        .last_scheduled = 0,
        .exit_code = 0,
        .exit_signal = 0,
        .pdeath_signal = 0,
        .pid = 0,
        .ppid = 0,
        .parent = NULL,
        .uid = 0,
        .gid = 0,
        .regular_tty = "",
        .io_tty = "",
        .tty = NULL,
        .locks = 0,
        .open_files = NULL,
        .entry_address = NULL,
        .descriptors = NULL,
        .next = NULL,
        .prev = NULL
    };

    cpu_t * lcpu = arch_get_bsp_cpu();
    lcpu->ctx = kmalloc(sizeof(cpu_context_t));
    memset(lcpu->ctx, 0x69, sizeof(cpu_context_t));
    memset(lcpu->ctx, 0, sizeof(cpu_context_t));
    lcpu->ctx->info = kmalloc(sizeof(struct cpu_context_info));
    memset(lcpu->ctx->info, 0, sizeof(struct cpu_context_info));
    current_process.cpu = lcpu;
    reloadGsFs();
    setGsBase((uint64_t)lcpu->ctx);
}

char * get_current_tty() {
    return current_process.tty;
}

void set_current_tty(char * tty) {
    current_process.tty = tty;
}

