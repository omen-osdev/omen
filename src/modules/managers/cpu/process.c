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

uint8_t is_in_vmarea(process_t* process, void * address) {
    struct vm_area * current = process->vm_areas;
    while (current) {
        if (address >= current->start && address < current->end) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

void create_vmarea(process_t* process, void * start, void * end, uint8_t flags) {
    struct vm_area * new_area = kmalloc(sizeof(struct vm_area));
    new_area->start = start;
    new_area->end = end;
    new_area->flags = flags;
    new_area->next = process->vm_areas;
    process->vm_areas = new_area;
}

void remove_vmarea(process_t* process, void * start) {
    struct vm_area * current = process->vm_areas;
    struct vm_area * previous = 0;

    while (current) {
        if (current->start == start) {
            if (previous) {
                previous->next = current->next;
            } else {
                process->vm_areas = current->next;
            }
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

void init_user_context(struct page_directory* pml4, process_t * task, void * init) {
    context_t * context = task->context;

    task->ustack_base = malloc(PROCESS_STACK_SIZE);
    memset(task->ustack_base, 0, PROCESS_STACK_SIZE);
    task->ustack = task->ustack_base + PROCESS_STACK_SIZE;
    mprotect(pml4, task->ustack_base, PROCESS_STACK_SIZE, VMM_USER_BIT | VMM_WRITE_BIT);

    task->kstack_base = kmalloc(KERNEL_STACK_SIZE);
    memset(task->kstack_base, 0, KERNEL_STACK_SIZE);
    task->kstack = task->kstack_base + KERNEL_STACK_SIZE;

    kprintf("Stack permissions after creating: %d\n", get_page_perms(pml4, task->ustack_base));
    kprintf("Is stack user access after creating: %d\n", is_user_access(pml4, task->ustack_base));
    kprintf("Kstack permissions after creating: %d\n", get_page_perms(pml4, task->kstack_base));
    kprintf("Is kstack user access after creating: %d\n", is_user_access(pml4, task->kstack_base));
    create_vmarea(task, task->ustack_base, task->ustack, VMM_USER_BIT | VMM_WRITE_BIT);
    create_vmarea(task, task->kstack_base, task->kstack, VMM_WRITE_BIT);

    //TODO: Initialize the stack
    newuctxcreat((uint64_t)&(task->ustack), (uint64_t)init);
    
    context->info = kmalloc(sizeof(struct cpu_context_info));
    memset(context->info, 0, sizeof(struct cpu_context_info));
    context->info->stack = (uint64_t) task->ustack;
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
    context->rsp = (uint64_t)task->ustack;
    
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

    task->context = kmalloc(sizeof(context_t)); 
    memset(task->context, 0, sizeof(context_t));
    struct page_directory * pd = get_pml4();
    init_user_context(pd, task, init);
    mprotect(pd, task->entry_address, 0x1000, VMM_USER_BIT); //TODO: change this for the elf loader
    task->context->cr3 = duplicate_current_pml4();
    kprintf("Process %d created\n", task->pid);
    kprintf("Stack permissions: %d\n", get_page_perms(task->context->cr3, task->ustack));
    kprintf("Is stack user access: %d\n", is_user_access(task->context->cr3, task->ustack));
    return task;
}

process_t * duplicate_process(process_t * parent) {
    process_t * task = &(process_list[process_count++]);
    memcpy(task, parent, sizeof(process_t));

    task->context = kmalloc(sizeof(context_t));
    task->context->info = kmalloc(sizeof(struct cpu_context_info));
    
    task->ustack_base = kmalloc(PROCESS_STACK_SIZE);
    memset(task->ustack_base, 0, PROCESS_STACK_SIZE);
    task->ustack = (parent->ustack - parent->ustack_base) + task->ustack_base;
    mprotect(parent->context->cr3, task->ustack_base, PROCESS_STACK_SIZE, VMM_WRITE_BIT | VMM_USER_BIT);
    memcpy(task->ustack_base, parent->ustack_base, PROCESS_STACK_SIZE);

    task->kstack_base = kmalloc(KERNEL_STACK_SIZE);
    memset(task->kstack_base, 0, KERNEL_STACK_SIZE);
    task->kstack = (parent->kstack - parent->kstack_base) + task->kstack_base;
    memcpy(task->kstack_base, parent->kstack_base, KERNEL_STACK_SIZE);

    memcpy(task->context, parent->context, sizeof(context_t));
    memcpy(task->context->info, parent->context->info, sizeof(struct cpu_context_info));
    memcpy(task->fxsave_region, parent->fxsave_region, 512);

    task->pid = get_next_pid();
    task->ppid = parent->pid;
    task->context->cr3 = duplicate_pd(parent->context->cr3, 0, 0, 0);

    return task;
}

void returnoexit() {
    panic("Returned from a process!\n");
}

void init_process(uint64_t address, uint64_t size) {
    kprintf("Hello there techzynth!\n");
    process_t * idle_proc = create_user_process(_idle);
    idle_proc->pid = 0;
    
    current_process = &process_list[0];
    current_process_index = 0;
    current_process->status = PROCESS_STATUS_RUNNING;

    current_process->cpu->cinfo->stack = current_process->kstack;
    current_process->cpu->ustack = current_process->ustack;
    tss_set_stack(current_process->cpu->tss, current_process->kstack, 0);
    tss_set_stack(current_process->cpu->tss, current_process->ustack, 3);
    __asm__("mov %0, %%rsp\n"
            "mov %1, %%cr3\n"
            "fxrstor %2\n"
            "ret\n" : : "r" (current_process->cpu->ustack), "r" (current_process->context->cr3), "m" (current_process->fxsave_region));
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
