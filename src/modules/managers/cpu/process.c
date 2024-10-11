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
#include <omen/apps/debug/debug.h>
//TODO: Delete this, we need an elf loader
#include <dummy/dummy.h>

extern void newctxswtch(context_t * old_task, context_t * new_task, void* fxsave, void* fxrstor);
extern void newctxcreat(void* rsp, void* intro);
extern void newuctxcreat(void* rsp, void* intro);

extern void reloadGsFs();
extern void setGsBase(uint64_t base);
extern void getGsBase(uint64_t * base);
extern void setKernelGsBase(uint64_t base);

//TODO: Jonbardo modify this to use ur linked list :D
process_t process_list[MAX_PROCESSES] = {0};
process_t *current_process = process_list;
uint32_t current_process_index = 0;
uint32_t process_count = 0;

void init_user_context(process_t * task, void * init) {
    context_t * context = task->context;

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
    
    __asm__ volatile("fxsave %0" : "=m" (task->fxsave_region));

}

int16_t get_next_pid() {
    int16_t npid = 1;

    while (npid < MAX_PROCESSES) {
        uint8_t found = 0;
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (process_list[i].pid == npid) {
                found = 1;
                break;
            }
        }

        if (!found) {
            return npid;
        } else {
            npid++;
        }
    }

    return -1;
}

process_t * create_user_process(void * init) {
    process_t * task = &(process_list[process_count++]);
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
    task->pid = get_next_pid();
    if (task->pid < 0) {
        panic("No more processes available\n");
    }
    task->locks = 0;
    task->open_files = 0;
    task->entry_address = init;
    task->tty = 0;
    task->descriptors = 0;
    task->parent = current_process;
    
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
    kprintf("Process %d created\n", task->pid);
    return task;    
}

void _idle() {
    while(1) {
        //syscall yield
        __asm__("syscall" : : "a" (24));
    }
}

void returnoexit() {
    panic("Returned from a process!\n");
}

void init_process(uint64_t address, uint64_t size) {
    process_t * idle_proc = create_user_process(_idle);
    idle_proc->pid = 0;
    mprotect(idle_proc->vm, idle_proc->entry_address, 0x1000, VMM_USER_BIT);
    process_t * init_proc = create_user_process(address);
    mprotect(init_proc->vm, init_proc->entry_address, size, VMM_USER_BIT);

    current_process = &process_list[0];
    current_process_index = 0;
    
    process_t dummy_process = {0};
    dummy_process.context = kmalloc(sizeof(context_t));
    memset(dummy_process.context, 0, sizeof(context_t));

    tss_set_stack(current_process->cpu->tss, current_process->context->info->stack, 3);
    newctxswtch(dummy_process.context, current_process->context, dummy_process.fxsave_region, current_process->fxsave_region);
}

process_t * sched() {
    //We advance one to avoid the current process
    current_process_index++;
    if (current_process_index >= process_count) {
        current_process_index = 0;
    }

    while (process_list[current_process_index].status != PROCESS_STATUS_READY && process_list[current_process_index].status != PROCESS_STATUS_RUNNING) {
        current_process_index++;
        if (current_process_index >= process_count) {
            current_process_index = 0;
        }
    }

    current_process = &process_list[current_process_index];
    return current_process;
}

void fork() {
    process_t * child = create_user_process(current_process->entry_address);
    yield_to(child);
}

void exit(int error_code) {
    current_process->status = PROCESS_STATUS_ZOMBIE;
    current_process->exit_code = error_code;
    yield();
}

void execve(const char * path, const char * argv, const char * envp) {
    //TODO: Implement an elf loader
}

void yield_to(process_t * next) {
    __asm__("cli");
    kprintf("Yielding to %d\n", next->pid);
    process_t * prev = current_process;
    current_process = next;
    tss_set_stack(current_process->cpu->tss, current_process->context->info->stack, 3);
    newctxswtch(prev->context, current_process->context, prev->fxsave_region, current_process->fxsave_region);
}

//TODO: Implement a real yield
void yield() {
    __asm__("cli");
    process_t * prev = current_process;
    sched();
    kprintf("Yielding from %d to %d\n", prev->pid, current_process->pid);
    tss_set_stack(current_process->cpu->tss, current_process->context->info->stack, 3);
    newctxswtch(prev->context, current_process->context, prev->fxsave_region, current_process->fxsave_region);
}

//TODO: This is awful
process_t * get_current_process() {
    return current_process;
}

char * get_current_tty() {
    return current_process->tty;
}

void set_current_tty(char * tty) {
    current_process->tty = tty;
}

