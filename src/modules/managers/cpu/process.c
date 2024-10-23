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
#include <idle/idle.h>

//Always inlined
extern void newuctxcreat(uint64_t rsp, uint64_t intro);

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
    mprotect_current(stack_top, PROCESS_STACK_SIZE, VMM_USER_BIT | VMM_WRITE_BIT);
    //TODO: Initialize the stack
    newuctxcreat((uint64_t)&stack, (uint64_t)init);
    
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
    task->context = kmalloc(sizeof(context_t)); 
    memset(task->context, 0, sizeof(context_t));
    init_user_context(task, init);
    mprotect_current(task->entry_address, 0x1000, VMM_USER_BIT); //TODO: change this for the elf loader
    duplicate_current_pml4(task->vm);
    kprintf("Process %d created\n", task->pid);
    kprintf("Stack permissions: %d\n", get_page_perms(task->vm, task->context->rsp));
    kprintf("Is stack user access: %d\n", is_user_access(task->vm, task->context->rsp));
    return task;    
}

process_t * duplicate_process(process_t * parent) {
    process_t * task = &(process_list[process_count++]);
    memcpy(task, parent, sizeof(process_t));

    task->vm = kmalloc(sizeof(struct page_directory));
    memset(task->vm, 0, sizeof(struct page_directory));

    task->context = kmalloc(sizeof(context_t));
    task->context->info = kmalloc(sizeof(struct cpu_context_info));
    
    memcpy(task->context, parent->context, sizeof(context_t));
    memcpy(task->context->info, parent->context->info, sizeof(struct cpu_context_info));
    memcpy(task->fxsave_region, parent->fxsave_region, 512);
    
    task->pid = get_next_pid();
    task->ppid = parent->pid;
    task->context->cr3 = (uint64_t) task->vm;

    duplicate_pml4(parent->vm, task->vm, 0, 0x200);
    set_cow(parent->vm, task->vm);
    return task;
}

void returnoexit() {
    panic("Returned from a process!\n");
}

void init_process(uint64_t address, uint64_t size) {
    process_t * idle_proc = create_user_process(_idle);
    idle_proc->pid = 0;
    //process_t * init_proc = create_user_process(address);
    //mprotect(init_proc->vm, init_proc->entry_address, size, VMM_USER_BIT);

    current_process = &process_list[0];
    current_process_index = 0;
    
    tss_set_stack(current_process->cpu->tss, current_process->context->info->stack, 3);
    //mprotect(current_process->vm, 0xffffffff8001a620, 0x1000, VMM_USER_BIT | VMM_WRITE_BIT);
    //fxrstor current_process->fxsave_region
    //cr3 = current_process->vm
    //rsp = current_process->context->rsp
    //ret

    __asm__("mov %0, %%rsp\n"
            "mov %1, %%cr3\n"
            "fxrstor %2\n"
            "ret\n" : : "r" (current_process->context->rsp), "r" (current_process->vm), "m" (current_process->fxsave_region));
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

int16_t fork() {   
    process_t * child = duplicate_process(current_process);
    child->status = PROCESS_STATUS_READY;
    child->context->rax = 0;
    return child->pid;
}

void exit(int error_code) {
    current_process->status = PROCESS_STATUS_ZOMBIE;
    current_process->exit_code = error_code;
    sched();
}

void execve(const char * path, const char * argv, const char * envp) {
    //TODO: Implement an elf loader
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

