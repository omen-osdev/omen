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
#include <omen/managers/cpu/vmarea.h>

#include <vfs/vfs.h>
#include <vfs/vfs_interface.h>

//Always inlined
extern void newuctxcreat(uint64_t rsp, uint64_t intro);
extern void newctxcreat(uint64_t rsp, uint64_t intro);
extern void reloadGsFs();
extern void getGsBase(uint64_t * base);
extern void getFsBase(uint64_t * base);
extern void setGsBase(uint64_t base);
extern void setFsBase(uint64_t base);
extern void setKernelGsBase(uint64_t base);

//Hardcoded functions
void returnoexit() {panic("Returned from a process!\n");}
void _idle() {panic("Stub running, exec failed!\n");}

//TODO: Jonbardo modify this to use ur linked list :D
process_t process_list[MAX_PROCESSES] = {0};
uint32_t process_count = 0;
uint32_t current_process_index = 0;

void init_stacks(process_t * task, thread_t * thread, uint64_t size, uint64_t entry) {
    if (size % 0x1000) {
        size = (size + 0x1000) & ~0xfff;
    }
    if (size > PROCESS_STACK_SIZE) {
        size = PROCESS_STACK_SIZE & ~0xfff;
    }

    struct stack stack;
    stackalloc(task->vmm, &stack, size);
    thread->ustack = stack.top;
    thread->ustack_base = stack.base;
    create_vmarea(task, thread->ustack_base, thread->ustack, VMM_USER_BIT | VMM_WRITE_BIT, 0, PAGE_SIZE_4KIB, -1, 0);
    kstackalloc(task->vmm, &stack, size);
    thread->kstack_base = stack.base;
    thread->kstack = stack.top;
    create_vmarea(task, thread->kstack_base, thread->kstack, VMM_WRITE_BIT, 0, PAGE_SIZE_4KIB, -1, 0);

    if (get_pml4() != task->vmm) {
        void * stack_physical = get_physical_address(task->vmm, thread->ustack_base);
        map_range(get_pml4(), thread->ustack_base, (uint64_t)stack_physical, PAGE_SIZE_4KIB, PROCESS_STACK_SIZE, VMM_WRITE_BIT);
    }
    
    newuctxcreat((uint64_t)&(thread->ustack), (uint64_t)entry);

    if (get_pml4() != task->vmm)
        unmap_range(get_pml4(), thread->ustack_base, PROCESS_STACK_SIZE);
}

context_t * create_context(void * cr3, void * ustack, void * kstack, void * init) {
    context_t * context = kmalloc(sizeof(context_t));

    context->fs_base = 0;
    context->gs_base = 0;
    __asm__ volatile("fxsave %0" : "=m" (context->fxsave_region));

    cpu_context_t * cpu_context = kmalloc(sizeof(cpu_context_t));
    memset(cpu_context, 0, sizeof(cpu_context_t));
    cpu_context->info = kmalloc(sizeof(struct cpu_context_info));
    memset(cpu_context->info, 0, sizeof(struct cpu_context_info));
    cpu_context->cr3 = (uint64_t)cr3;
    cpu_context->info->kstack = (uint64_t) kstack;
    cpu_context->info->cs = get_user_code_selector();
    cpu_context->info->ss = get_user_data_selector();
    cpu_context->info->thread = 0;
    cpu_context->rax = 1;
    cpu_context->rbx = 2;
    cpu_context->rcx = 3;
    cpu_context->rdx = 4;
    cpu_context->rsi = 5;
    cpu_context->rdi = 6;
    cpu_context->rbp = 7;
    cpu_context->r8 = 8;
    cpu_context->r9 = 9;
    cpu_context->r10 = 10;
    cpu_context->r11 = 11;
    cpu_context->r12 = 12;
    cpu_context->r13 = 13;
    cpu_context->r14 = 14;
    cpu_context->r15 = 15;
    cpu_context->interrupt_number = 0;
    cpu_context->error_code = 0;    
    cpu_context->rip = (uint64_t)init;
    cpu_context->rflags = PROCESS_STARTUP_RFLAGS;
    cpu_context->cs = get_user_code_selector();
    cpu_context->ss = get_user_data_selector();
    cpu_context->rsp = (uint64_t)ustack;

    context->cpu_context = cpu_context;

    return context;
}

void init_thread(process_t * task, void * init) {
    if (task->thread_count >= MAX_THREADS) {
        panic("Too many threads\n");
    }

    thread_t * thread = &(task->threads[task->thread_count++]);
    memset(thread, 0, sizeof(thread_t));

    init_stacks(task, thread, task->stack_max_size, init);
    thread->process = task;
    thread->context = create_context(from_identity_map(task->vmm), thread->ustack, thread->kstack, init);
    thread->entry = init;
    thread->id = task->thread_count - 1;
    thread->core_id = arch_get_bsp_cpu()->core_id;
    thread->status = THREAD_STATUS_READY;
    thread->syscall_ready = 0;

    thread->pending_signal = 0;

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

process_t * duplicate_process(thread_t * parent_thread) {
    process_t * parent = parent_thread->process;
    if (process_count >= MAX_PROCESSES) {
        panic("No more processes available\n");
    }
    process_t * task = &(process_list[process_count++]);
    vmarea_sync_all_files(parent);
    memcpy(task, parent, sizeof(process_t));
    task->thread_count = 1;
    task->current_thread = 0;
    task->main_thread = 0;
    task->pid = get_next_pid();
    task->ppid = parent->pid;
    memset(task->threads, 0, sizeof(thread_t) * MAX_THREADS);
    memcpy(&(task->threads[0]), parent_thread, sizeof(thread_t));
    
    thread_t * main_thread = &(task->threads[0]);

    main_thread->process = task;
    main_thread->context = kmalloc(sizeof(context_t));
    memcpy(main_thread->context, parent_thread->context, sizeof(context_t));
    main_thread->context->cpu_context = kmalloc(sizeof(cpu_context_t));
    memcpy(main_thread->context->cpu_context, parent_thread->context->cpu_context, sizeof(cpu_context_t));
    main_thread->context->cpu_context->info = kmalloc(sizeof(struct cpu_context_info));
    memcpy(main_thread->context->cpu_context->info, parent_thread->context->cpu_context->info, sizeof(struct cpu_context_info));

    duplicate_vmareas(parent, task);
    memcpy(task->open_files, parent->open_files, sizeof(int)*MAX_OPEN_FILES);
    task->open_files_count = parent->open_files_count;
    memcpy(main_thread->context->fxsave_region, parent_thread->context->fxsave_region, 512);

    main_thread->context->fs_base = parent_thread->context->fs_base;
    main_thread->context->gs_base = parent_thread->context->gs_base;

    task->vmm = vmm_copy(parent->vmm);
    main_thread->context->cpu_context->cr3 = from_identity_map(task->vmm);
    vmm_copy_stack(task->vmm, parent_thread->ustack_base, task->stack_max_size, VMM_USER_BIT | VMM_WRITE_BIT);
    vmm_copy_stack(task->vmm, parent_thread->kstack_base, task->stack_max_size, VMM_WRITE_BIT);
    engrave_vmareas(task, parent);

    kprintf("Process %d duplicated\n", task->pid);
    return task;
}


void alter_process_on_exec(process_t * task, void * init) {
    process_t saved_task;
    memcpy(&saved_task, task, sizeof(process_t));
    memset(task, 0, sizeof(process_t));

    task->vmm = saved_task.vmm;
    task->vm_areas = 0;

    memset(task->threads, 0, sizeof(thread_t) * MAX_THREADS);
    task->thread_count = 0;
    task->current_thread = 0;
    task->main_thread = 0;

    task->heap_base = 0;
    task->heap_end = 0;
    task->heap_max_size = 0;

    task->nice = 0;
    task->privilege = 0;
    task->current_nice = 0;
    task->exit_code = 0;
    task->exit_signal = 0;
    task->pdeath_signal = 0;

    task->sleep_time = 0;
    task->cpu_time = 0;
    task->last_scheduled = 0;
    task->locks = 0;

    task->stack_max_size = PROCESS_STACK_SIZE;

    task->pid = saved_task.pid;
    task->parent = saved_task.parent;
    task->uid = saved_task.uid;
    task->gid = saved_task.gid;
    task->ppid = saved_task.ppid;

    memcpy(task->open_files, saved_task.open_files, sizeof(int)*MAX_OPEN_FILES);
    task->open_files_count = saved_task.open_files_count;
    task->regular_tty = saved_task.regular_tty;
    task->io_tty = saved_task.io_tty;
    task->ctty = saved_task.ctty;
    
    task->entry_address = init;

    init_thread(task, init);

    kprintf("Exec: Process %d created\n", task->pid);
}

int exec(process_t * task, char const *path) {

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

    vmm_unmap_userspace(task->vmm);
    void * entry = elf_load_elf(task->vmm, buf, size);
    alter_process_on_exec(task, entry);
    kfree(buf);
    return 0;
}

void execve(process_t* task, const char * path, const char * argv, const char * envp) {
    (void)argv;
    (void)envp;
    exec(task, path);
}

uint8_t sched_thread(process_t * task) {
    int current_thread_index = task->current_thread + 1;
    if (current_thread_index >= task->thread_count) {
        current_thread_index = 0;
    }

    while (task->threads[current_process_index].status != THREAD_STATUS_READY && task->threads[current_process_index].status != THREAD_STATUS_RUNNING) {
        current_thread_index++;
        if (current_thread_index >= task->thread_count) {
            current_thread_index = 0;
        }
        if (current_thread_index == task->current_thread) {
            return 0;
        }
    }

    task->current_thread = current_thread_index;
    thread_t * thread = &(task->threads[task->current_thread]);
    thread->status = THREAD_STATUS_RUNNING;

    return 1;
}

process_t * sched() {
    //We advance one to avoid the current process
    current_process_index++;
    if (current_process_index >= process_count) {
        current_process_index = 0;
    }

    while (!sched_thread(&process_list[current_process_index])) {
        current_process_index++;
        if (current_process_index >= process_count) {
            current_process_index = 0;
        }
    }
    
    process_signals(&process_list[current_process_index]);
    return &process_list[current_process_index];
}

int16_t fork(thread_t * ct) {   
    process_t * child = duplicate_process(ct);
    thread_t * main_thread = &(child->threads[0]);
    main_thread->status = THREAD_STATUS_READY;
    main_thread->context->cpu_context->rax = 0;
    return child->pid;
}

void exit(process_t* task, int error_code) {
    for (int i = 0; i < task->thread_count; i++) {
        thread_t * thread = &(task->threads[i]);
        thread->status = THREAD_STATUS_ZOMBIE;
    }
    task->exit_code = error_code;
    sched();
}

process_t * create_user_process(struct page_directory* pd, void * init, char * tty) {
    process_t * task = &(process_list[process_count++]);
    memset(task, 0, sizeof(process_t));

    task->vmm = vmm_copy_kernel(pd);
    task->vm_areas = 0;

    memset(task->threads, 0, sizeof(thread_t) * MAX_THREADS);
    task->thread_count = 0;
    task->current_thread = 0;
    task->main_thread = 0;
    task->heap_base = 0;
    task->heap_end = 0;
    task->heap_max_size = 0;
    
    task->nice = 0;
    task->privilege = 0;
    task->current_nice = 0;
    task->exit_code = 0;
    task->exit_signal = 0;
    task->pdeath_signal = 0;

    task->sleep_time = 0;
    task->cpu_time = 0;
    task->last_scheduled = 0;
    task->locks = 0;

    task->stack_max_size = PROCESS_STACK_SIZE;

    task->pid = get_next_pid();
    if (task->pid < 0) {
        panic("No more processes available\n");
    }
    task->parent = get_current_process();
    if (task->parent) {
        task->uid = task->parent->uid;
        task->gid = task->parent->gid;
        task->ppid = task->parent->pid;
    } else {
        task->uid = 0;
        task->gid = 0;
        task->ppid = 0;
    }

    memset(task->open_files, 0, sizeof(int)*MAX_OPEN_FILES);
    task->open_files_count = 0;
    open_stdfiles(task, tty);

    task->regular_tty = task->open_files[PROCFILE_STDIN];
    task->io_tty = task->open_files[PROCFILE_STDERR];
    task->ctty = &(task->regular_tty);
    
    task->entry_address = init;

    init_thread(task, init);

    kprintf("Process %d created\n", task->pid);
    return task;
}

void init_process(const char * _init_path, const char * _idle_path, char * tty) {
    process_t * init_proc = create_user_process(get_pml4(), (void*)_idle, tty);
    init_proc->pid = 0;
    current_process_index = 0;
    exec(init_proc, _init_path);

    thread_t * main_thread = &(get_current_process()->threads[0]);
    main_thread->status = THREAD_STATUS_RUNNING;

    struct tss * tss = arch_get_cpu(main_thread->core_id)->tss;
    tss_set_stack(tss, main_thread->kstack, 0);
    tss_set_stack(tss, main_thread->ustack, 3);
    __asm__("mov %0, %%rsp\n"
            "mov %1, %%cr3\n"
            "fxrstor %2\n"
            "ret\n" : : "r" (main_thread->ustack), "r" (main_thread->context->cpu_context->cr3), "m" (main_thread->context->fxsave_region));
    panic("Returned from init process\n");
}

thread_t * get_current_thread() {
    process_t * current_process = get_current_process();
    return &(current_process->threads[current_process->current_thread]);  
}

process_t * get_current_process() {
    return &process_list[current_process_index];
}