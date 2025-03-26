#include <generic/config.h>
#include <omen/apps/panic/panic.h>
#include <omen/managers/cpu/process.h>
#include <omen/libraries/crypto/md5.h>
#include <omen/managers/mem/vmm.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/std/stddef.h>
#include <omen/libraries/executables/loader.h>
#include <omen/hal/arch/x86/cpu.h>
#include <omen/hal/arch/x86/gdt.h>
#include <omen/hal/arch/x86/msr.h>
#include <omen/hal/arch/x86/syscall.h>
#include <omen/apps/debug/debug.h>

#include <vfs/vfs.h>
#include <vfs/vfs_interface.h>

//Always inlined
extern void newuctxcreat(uint64_t rsp, uint64_t intro);
extern void newctxcreat(uint64_t rsp, uint64_t intro);

extern void reloadGsFs();
extern void setGsBase(uint64_t base);
extern void getGsBase(uint64_t * base);
extern void setKernelGsBase(uint64_t base);

//TODO: Jonbardo modify this to use ur linked list :D
process_t process_list[MAX_PROCESSES] = {0};
process_t *current_process = process_list;
uint32_t current_process_index = 0;
uint32_t process_count = 0;
char init_path[0x1000] __attribute__((aligned(0x1000)));
char idle_path[0x1000] __attribute__((aligned(0x1000)));

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

void duplicate_vmareas(process_t * old, process_t * new) {
    struct vm_area * current = old->vm_areas;
    while (current) {
        create_vmarea(new, current->start, current->end, current->flags);
        current = current->next;
    }
}

void set_tty(process_t * task, char* tty) {
    memset(task->regular_tty, 0, 32);
    if (strlen(tty) > 32) {
        panic("TTY name too long\n");
    } else {
        strncpy(task->regular_tty, tty, strlen(tty));
        task->regular_tty[strlen(tty)] = '\0';
        task->tty = task->regular_tty;
    }
}

void init_stack(struct page_directory* pd, process_t * task, uint64_t size, uint8_t is_userland_stack) {
    if (size % 0x1000) {
        size = (size + 0x1000) & ~0xfff;
    }

    struct stack stack;
    if (is_userland_stack) {
        stackalloc(&stack, size);
        task->ustack = stack.top;
        task->ustack_base = stack.base;
        memset(to_identity_map(task->ustack_base), 0, size);
        map_range(pd, task->ustack_base, task->ustack_base, 0x1000, size);
        mprotect(pd, task->ustack_base, size, VMM_USER_BIT | VMM_WRITE_BIT);
        create_vmarea(task, task->ustack_base, task->ustack, VMM_USER_BIT | VMM_WRITE_BIT);
    } else {
        kstackalloc(&stack, size);
        task->kstack_base = stack.base;
        task->kstack = stack.top;
        memset(task->kstack_base, 0, size);
        map_range(pd, task->kstack_base, task->kstack_base, 0x1000, size);
        mprotect(pd, task->kstack_base, size, VMM_WRITE_BIT);
        create_vmarea(task, task->kstack_base, task->kstack, VMM_WRITE_BIT);
    }
}

void create_context(process_t * task, struct page_directory* pd, void * ustack, void * kstack, void * init) {
    task->context = kmalloc(sizeof(cpu_context_t));
    memset(task->context, 0, sizeof(cpu_context_t));
    task->context->info = kmalloc(sizeof(struct cpu_context_info));
    memset(task->context->info, 0, sizeof(struct cpu_context_info));
    task->context->cr3 = pd;
    task->context->info->kstack = (uint64_t) kstack;
    task->context->info->cs = get_user_code_selector();
    task->context->info->ss = get_user_data_selector();
    task->context->info->thread = 0;
    task->context->rax = 1;
    task->context->rbx = 2;
    task->context->rcx = 3;
    task->context->rdx = 4;
    task->context->rsi = 5;
    task->context->rdi = 6;
    task->context->rbp = 7;
    task->context->r8 = 8;
    task->context->r9 = 9;
    task->context->r10 = 10;
    task->context->r11 = 11;
    task->context->r12 = 12;
    task->context->r13 = 13;
    task->context->r14 = 14;
    task->context->r15 = 15;
    task->context->interrupt_number = 0;
    task->context->error_code = 0;    
    task->context->rip = (uint64_t)init;
    task->context->rflags = PROCESS_STARTUP_RFLAGS;
    task->context->cs = get_user_code_selector();
    task->context->ss = get_user_data_selector();
    task->context->rsp = (uint64_t)ustack;
}

void init_user_context(struct page_directory* pd, process_t * task, void * init, uint8_t trampoline) {

    init_stack(pd, task, PROCESS_STACK_SIZE, 1);
    init_stack(pd, task, PROCESS_STACK_SIZE, 0);

    //TODO: Initialize the stack
    if (trampoline)
        newuctxcreat((uint64_t)&(task->ustack), (uint64_t)init);
    else
        newctxcreat((uint64_t)&(task->ustack), (uint64_t)init);

    create_context(task, pd, task->ustack, task->kstack, init);
    
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

void open_stdfiles(process_t *task, char * tty) {
    int stdin, stdout, stderr;
    stdin = vfs_file_open(tty, O_RDONLY, 0);
    if (stdin < 0) {
        panic("Failed to open stdin\n");
    }
    stdout = vfs_file_open(tty, O_WRONLY, 0);
    if (stdout < 0) {
        panic("Failed to open stdout\n");
    }
    stderr = vfs_file_open(tty, O_WRONLY, 0);
    if (stderr < 0) {
        panic("Failed to open stderr\n");
    }

    task->open_files[task->open_files_count++] = stdin;
    task->open_files[task->open_files_count++] = stdout;
    task->open_files[task->open_files_count++] = stderr;
}

process_t * create_user_process(void * init, char * tty) {
    process_t * task = &(process_list[process_count++]);
    memset(task, 0, sizeof(process_t));
    task->status = PROCESS_STATUS_READY;
    task->signal_pending = 0;
    task->nice = 0;
    task->privilege = 0;
    task->core_id = arch_get_bsp_cpu()->core_id;
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
    memset(task->open_files, 0, sizeof(int)*MAX_OPEN_FILES);
    task->open_files_count = 0;
    open_stdfiles(task, tty);
    task->entry_address = init;
    set_tty(task, tty);
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

    struct page_directory * pd = vmm_copy_kernel(get_pml4());

    init_user_context(pd, task, init, 1);

    kprintf("Process %d created\n", task->pid);
    return task;
}

process_t * duplicate_process(process_t * parent) {
    process_t * task = &(process_list[process_count++]);
    memcpy(task, parent, sizeof(process_t));

    task->context = kmalloc(sizeof(cpu_context_t));
    memcpy(task->context, parent->context, sizeof(cpu_context_t));
    task->context->info = kmalloc(sizeof(struct cpu_context_info));
    memcpy(task->context->info, parent->context->info, sizeof(struct cpu_context_info));
    duplicate_vmareas(parent, task);
    memcpy(task->open_files, parent->open_files, sizeof(int)*MAX_OPEN_FILES);
    task->open_files_count = parent->open_files_count;
    memcpy(task->fxsave_region, parent->fxsave_region, 512);

    task->pid = get_next_pid();
    task->ppid = parent->pid;
    task->context->cr3 = vmm_copy(parent->context->cr3);
    vmm_copy_stack(task->context->cr3, parent->ustack_base, PROCESS_STACK_SIZE, VMM_USER_BIT | VMM_WRITE_BIT);
    vmm_copy_stack(task->context->cr3, parent->kstack_base, PROCESS_STACK_SIZE, VMM_WRITE_BIT);
    kprintf("Process %d duplicated\n", task->pid);
    return task;
}

void returnoexit() {
    panic("Returned from a process!\n");
}

uint64_t _internal_syscall(uint64_t syscall_number, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    unsigned long long ret;
    __asm__ volatile ("syscall" : "=a" (ret) : "a" (syscall_number), "D" (arg1), "S" (arg2), "d" (arg3), "r" (arg4), "r" (arg5), "r" (arg6) : "memory");
    return ret;
}

void _idle() {
    panic("Stub running, exec failed!\n");
}

void init_process(const char * _init_path, const char * _idle_path, char * tty) {
    mprotect_current(init_path, 0x1000, VMM_USER_BIT | VMM_WRITE_BIT);
    memset(init_path, 0, 0x1000);
    strcpy(init_path, _init_path);
    mprotect_current(idle_path, 0x1000, VMM_USER_BIT | VMM_WRITE_BIT);
    memset(idle_path, 0, 0x1000);
    strcpy(idle_path, _idle_path);

    process_t * idle_proc = create_user_process((void*)_idle, tty);
    idle_proc->pid = 0;
    current_process = idle_proc;
    current_process_index = 0;
    exec(idle_path);

    process_t * init_proc = create_user_process((void*)_idle, tty);
    init_proc->pid = 1;
    current_process = init_proc;
    current_process_index = 1;
    exec(init_path);

    current_process = &process_list[1];
    current_process_index = 1;
    current_process->status = PROCESS_STATUS_RUNNING;

    struct tss * tss = arch_get_cpu(current_process->core_id)->tss;
    tss_set_stack(tss, current_process->kstack, 0);
    tss_set_stack(tss, current_process->ustack, 3);
    void * cr3 = get_physical_address(current_process->context->cr3, current_process->context->cr3);
    __asm__("mov %0, %%rsp\n"
            "mov %1, %%cr3\n"
            "fxrstor %2\n"
            "ret\n" : : "r" (current_process->ustack), "r" (cr3), "m" (current_process->fxsave_region));
    panic("Returned from init process\n");
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

//TODO: This is awful
process_t * get_current_process() {
    return current_process;
}

void alter_process_on_exec(process_t * task, void * init) {
    process_t saved_task;
    memcpy(&saved_task, task, sizeof(process_t));
    memset(task, 0, sizeof(process_t));
    task->status = PROCESS_STATUS_READY;
    task->signal_pending = 0;
    task->nice = 0;
    task->privilege = 0;
    task->core_id = arch_get_bsp_cpu()->core_id;
    task->cpu_time = 0;
    task->last_scheduled = 0;
    task->sleep_time = 0;
    task->exit_code = 0;    
    task->exit_signal = 0;
    task->pdeath_signal = 0;
    task->pid = saved_task.pid;
    if (task->pid < 0) {
        panic("No more processes available\n");
    }
    task->locks = 0;
    memcpy(task->open_files, saved_task.open_files, sizeof(int)*MAX_OPEN_FILES);
    task->open_files_count = saved_task.open_files_count;
    task->entry_address = init;
    memcpy(task->regular_tty, saved_task.regular_tty, 32);
    task->tty = task->regular_tty;
    memcpy(task->io_tty, saved_task.io_tty, 32);
    task->parent = saved_task.parent;
    
    task->uid = saved_task.uid;
    task->gid = saved_task.gid;
    task->ppid = saved_task.ppid;

    init_user_context(saved_task.context->cr3, task, init, 1);
    kprintf("Exec: Process %d created\n", task->pid);
}

int exec(char const *path) {
    process_t * task = get_current_process();

    char * dynpath = kmalloc(256);
    strcpy(dynpath, path);
    int fd = vfs_file_open(dynpath, 0, 0);
    if (fd < 0) {
        kprintf("Could not open file %s\n", dynpath);
        return -1;
    }
    kfree(dynpath);

    vfs_file_seek(fd, 0, 0x2); //SEEK_END
    uint64_t size = vfs_file_tell(fd);
    vfs_file_seek(fd, 0, 0x0); //SEEK_SET

    uint8_t* buf = kmalloc(size);
    memset(buf, 0, size);

    vfs_file_read(fd, buf, size);
    vfs_file_close(fd);

    unsigned char *md5_buffer = kmalloc(16);
    memset(md5_buffer, 0, 16);
    MD5_Digest(md5_buffer, buf, size);
    kprintf("MD5: ");
    for (int i = 0; i < 16; i++) {
        kprintf("%x", md5_buffer[i]);
    }
    kprintf("\n");

    elf_readelf(buf, size);
    kfree(md5_buffer);
    vmm_unmap_userspace(task->context->cr3);
    void * entry = elf_load_elf(task->context->cr3, buf, size);
    kfree(buf);
    alter_process_on_exec(task, entry);
    return 0;
}

void execve(const char * path, const char * argv, const char * envp) {
    (void)argv;
    (void)envp;
    exec(path);
}

char * get_current_tty() {
    return current_process->tty;
}

void set_current_tty(char * tty) {
    set_tty(current_process, tty);
}
