#include <generic/config.h>
#include <omen/apps/panic/panic.h>
#include <omen/managers/cpu/process.h>
#include <omen/libraries/crypto/md5.h>
#include <omen/managers/mem/vmm.h>
#include <omen/managers/cpu/signal.h>
#include <omen/apps/debug/debug.h>
#include <omen/managers/dev/pit.h>
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
#include <omen/managers/cpu/sline.h>

#include <vfs/vfs.h>
#include <vfs/vfs_interface.h>

//Always inlined
extern void newuctxcreat(uint64_t rsp, uint64_t intro);
extern void newctxcreat(uint64_t rsp, uint64_t intro);
extern void newsctxcreat(uint64_t rsp, uint64_t intro, uint64_t signo, void * sigact, void * uctx);
extern void _signal_trampoline();
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
process_t * current_process = NULL;
int process_count = 0;

void * create_args_env_aux(void * protostack_buffer, uint64_t size, char ** argv, char ** envp, struct auxv* auxv) {
    int argc = 0;
    int envc = 0;
    int auxc = 0;

    // Count the number of arguments
    uint64_t len = 0;
    if (argv != 0)
        for (argc = 0; argv[argc] != NULL; argc++) {len+= strlen(argv[argc]) + 1;}
    if (envp != 0)
        for (envc = 0; envp[envc] != NULL; envc++) {len+= strlen(envp[envc]) + 1;}
    if (auxv != 0)
        for (auxc = 0; (auxv[auxc].a_type != AT_NULL); auxc++) {len+= sizeof(struct auxv);}
    len += 6*8;

    if ((len+0xf) > size) {
        panic("Protostack too big\n");
    }

    uint64_t * protostack = (uint64_t)((uint64_t)protostack_buffer - len);
    //Make sure protostack is aligned to 16 bytes
    protostack = (uint64_t*)((uint64_t)protostack & ~0xf);

    memset(protostack, 0, len);

    protostack[0] = argc;
    //                                      argc args  null envs  null   auxv+null
    uint64_t protostack_offset = (uint64_t)((1 + argc + 1 + envc + 1 + ((auxc+1)*2)) * 8);

    // Copy the arguments to the protostack
    for (int i = 0; i < argc; i++) {
        protostack[i + 1] = (uint64_t)protostack + (uint64_t)protostack_offset;
        kprintf("protostack[%d] Copying argv[%d] at %p (%s) to final addr:%p\n", i+1, i, argv[i], argv[i], protostack[i + 1]);
        memcpy(protostack[i + 1], argv[i], strlen(argv[i]) + 1);
        kprintf("Protostack string: %s argv string: %s\n", (char*)protostack[i + 1], argv[i]);
        protostack_offset += (uint64_t)(strlen(argv[i]) + 1);
    }
    protostack[argc + 1] = 0;
    // Copy the environment variables to the protostack
    for (int i = 0; i < envc; i++) {
        protostack[i + argc + 2] = (uint64_t)protostack + (uint64_t)protostack_offset;
        kprintf("protostack[%d] Copying envp[%d] at %p (%s) to final addr:%p\n", i+argc+2, i, envp[i], envp[i], protostack[i + argc + 2]);
        memcpy(protostack[i + argc + 2], envp[i], strlen(envp[i]) + 1);
        kprintf("Protostack string: %s envp string: %s\n", (char*)protostack[i + argc + 2], envp[i]);
        protostack_offset += (uint64_t)(strlen(envp[i]) + 1);
    }
    protostack[argc + envc + 2] = 0;
    // Copy the auxv to the protostack
    for (int i = 0; i < auxc; i++) {
        struct auxv * aux = (struct auxv*)&(protostack[i*2 + argc + envc + 3]);
        aux->a_type = auxv[i].a_type;
        aux->a_val = auxv[i].a_val;
        kprintf("protostack[%d] Copying auxv[%d] at %p (%s) to final addr:%p\n", (i*2)+argc+envc+3, i, auxv[i].a_val, get_auxv_string(auxv[i].a_type), &protostack[i*2 + argc + envc + 3]);
        kprintf("Protostack type: %s value: %llx\n", get_auxv_string(auxv[i].a_type), auxv[i].a_val);
    }
    struct auxv * aux = (struct auxv*)&(protostack[argc + envc + 3 + auxc*2]);
    aux->a_type = AT_NULL;
    aux->a_val = 0;
    //Add a NULL terminator to the protostack
    protostack[argc + envc + auxc*2 + 4] = 0;

    kprintf("protostack at %p size: %d\n", protostack, len);
    kprintf("argc: %d envc: %d auxc: %d\n", argc, envc, auxc);
    char ** argv_ptr = (char **)&(protostack[1]);
    char ** envp_ptr = (char **)&(protostack[argc + 2]);
    struct auxv * auxv_ptr = (struct auxv *)&(protostack[argc + envc + 3]);

    for (int i = 0; i < argc; i++) {
        kprintf("argv[%d]: %s\n", i, argv_ptr[i]);
    }

    for (int i = 0; i < envc; i++) {
        kprintf("envp[%d]: %s\n", i, envp_ptr[i]);
    }

    for (int i = 0; i < auxc; i++) {
        kprintf("auxv[%d]: TYPE: %s VAL: %llx\n", i, get_auxv_string(auxv_ptr[i].a_type), auxv_ptr[i].a_val);
    }

    return protostack;
}

void * get_vdso_base() {
    return (void*)VMM_REGION_U_VDSO;
}

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
    
    thread->ustack = create_args_env_aux(thread->ustack, PROCESS_STACK_SIZE, task->argv, task->envp, task->auxv);
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

void restore_signal_context(thread_t * thread, cpu_context_t * ctx) {
    thread->ustack = thread->altstack_saved_stack;
    thread->ustack_base = thread->altstack_saved_base;
    thread->altstack_saved_stack = 0;
    thread->altstack_saved_base = 0;

    uint64_t vdso_signo_region;
    uint64_t vdso_sigctxt_region;
    uint64_t vdso_sigact_region;
    int64_t size;
    vdso_get_data(thread->process->vdso, VDSO_ENTRY_SIGNAL_SIGNO, (void**)&vdso_signo_region, &size);
    vdso_get_data(thread->process->vdso, VDSO_ENTRY_SIGNAL_SIGCTXT, (void**)&vdso_sigctxt_region, &size);
    vdso_get_data(thread->process->vdso, VDSO_ENTRY_SIGNAL_SIGACTION, (void**)&vdso_sigact_region, &size);
    if (vdso_signo_region == 0 || vdso_sigctxt_region == 0 || vdso_sigact_region == 0) {
        kprintf("Failed to get vdso signal regions\n");
        return;
    }
    vdso_free_region(thread->process->vdso, vdso_signo_region);
    vdso_free_region(thread->process->vdso, vdso_sigctxt_region);
    vdso_free_region(thread->process->vdso, vdso_sigact_region);
    vdso_set_data(thread->process->vdso, VDSO_ENTRY_SIGNAL_SIGNO, NULL, 0);
    vdso_set_data(thread->process->vdso, VDSO_ENTRY_SIGNAL_SIGCTXT, NULL, 0);
    vdso_set_data(thread->process->vdso, VDSO_ENTRY_SIGNAL_SIGACTION, NULL, 0);

    thread->context = kmalloc(sizeof(context_t));
    memcpy(thread->context, thread->signal_context, sizeof(context_t));
    thread->context->cpu_context = kmalloc(sizeof(cpu_context_t));
    memcpy(thread->context->cpu_context, thread->signal_context->cpu_context, sizeof(cpu_context_t));
    thread->context->cpu_context->info = kmalloc(sizeof(struct cpu_context_info));
    memcpy(thread->context->cpu_context->info, thread->signal_context->cpu_context->info, sizeof(struct cpu_context_info));

    kfree(thread->signal_context->cpu_context->info);
    kfree(thread->signal_context->cpu_context);
    kfree(thread->signal_context);

    thread->signal_context = NULL;
    
}

void awake_parent(thread_t * thread) {
    if (thread->process->parent != NULL) {
        for (int i = 0; i < thread->process->parent->thread_count; i++) {
            thread_t * parent_thread = &(thread->process->parent->threads[i]);
            if (parent_thread->waiting == 1) {
                parent_thread->waiting = 0;
                wakeup(SLEEP_WAITPID);
            }
        }
    }
}

void create_signal_context(thread_t * thread, int signo, struct sigaction * sigact, cpu_context_t * ctx) {

    if (signo == SIGKILL) {
        thread->process->global_status = PROCESS_STATUS_ZOMBIE;
        awake_parent(thread);
        return;
    }
    if (signo == SIGSTOP) {
        thread->process->global_status = PROCESS_STATUS_SIGSTOP;
        awake_parent(thread);
        return;
    }
    if (signo == SIGCONT && thread->process->global_status == PROCESS_STATUS_SIGSTOP) {
        thread->process->global_status = PROCESS_STATUS_SIGCONT;
        awake_parent(thread);
    }

    void * vdso_signal_trampoline = get_signal_trampoline(thread->process);
    if (vdso_signal_trampoline == NULL) {
        kprintf("Failed to get vdso signal trampoline\n");
        return;
    }

    thread->signal_context = kmalloc(sizeof(context_t));
    memcpy(thread->signal_context, thread->context, sizeof(context_t));
    thread->signal_context->cpu_context = kmalloc(sizeof(cpu_context_t));
    memcpy(thread->signal_context->cpu_context, thread->context->cpu_context, sizeof(cpu_context_t));
    thread->signal_context->cpu_context->info = kmalloc(sizeof(struct cpu_context_info));
    memcpy(thread->signal_context->cpu_context->info, thread->context->cpu_context->info, sizeof(struct cpu_context_info));

    uint64_t size = PROCESS_STACK_SIZE;
    if (thread->altstack == 0) {
        struct stack stack;
        stackalloc(thread->process->vmm, &stack, size);
        thread->altstack = stack.top;
        thread->altstack_base = stack.base;
        create_vmarea(thread->process, thread->ustack_base, thread->ustack, VMM_USER_BIT | VMM_WRITE_BIT, 0, PAGE_SIZE_4KIB, -1, 0);
    } else {
        size = (uint64_t)thread->altstack - (uint64_t)thread->altstack_base;
        if (size % 0x1000) {
            size = (size + 0x1000) & ~0xfff;
        }
    }

    thread->altstack_saved_stack = thread->ustack;
    thread->altstack_saved_base = thread->ustack_base;
    thread->ustack = thread->altstack;
    thread->ustack_base = thread->altstack_base;

    if (get_pml4() != thread->process->vmm) {
        void * stack_physical = get_physical_address(thread->process->vmm, thread->ustack_base);
        map_range(get_pml4(), thread->ustack_base, (uint64_t)stack_physical, PAGE_SIZE_4KIB, size, VMM_WRITE_BIT);
    }
    
    int * uaccess_signo = vdso_allocate_region(thread->process->vdso, sizeof(int));
    memcpy(uaccess_signo, &signo, sizeof(int));
    cpu_context_t * uaccess_ctx = vdso_allocate_region(thread->process->vdso, sizeof(cpu_context_t));
    memcpy(uaccess_ctx, ctx, sizeof(cpu_context_t));
    uaccess_ctx->info = vdso_allocate_region(thread->process->vdso, sizeof(struct cpu_context_info));
    memcpy(uaccess_ctx->info, ctx->info, sizeof(struct cpu_context_info));
    struct sigaction * uaccess_sigact = vdso_allocate_region(thread->process->vdso, sizeof(struct sigaction));
    memcpy(uaccess_sigact, sigact, sizeof(struct sigaction));

    if (vdso_set_data(thread->process->vdso, VDSO_ENTRY_SIGNAL_SIGNO, (void*)uaccess_signo, VDSO_REGION_SIZE(8)))
        panic("Failed to set vdso signal signo\n");
    if (vdso_set_data(thread->process->vdso, VDSO_ENTRY_SIGNAL_SIGCTXT, (void*)uaccess_ctx, VDSO_REGION_SIZE(8)))
        panic("Failed to set vdso signal context\n");
    if (vdso_set_data(thread->process->vdso, VDSO_ENTRY_SIGNAL_SIGACTION, (void*)uaccess_sigact, VDSO_REGION_SIZE(8)))
        panic("Failed to set vdso signal action\n");

    newsctxcreat((uint64_t)&(thread->ustack), (uint64_t)vdso_signal_trampoline, (uint64_t)*uaccess_signo, (void*)uaccess_sigact, (void *)uaccess_ctx);

    if (get_pml4() != thread->process->vmm)
        unmap_range(get_pml4(), thread->ustack_base, size);

    ctx->rip = (uint64_t)_signal_trampoline;
    ctx->rsp = (uint64_t)thread->ustack;
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
    thread->sigprocmask = 0;
    thread->sigsuspend_mask = 0;
    thread->altstack = 0;
    thread->altstack_base = 0;
    thread->altstack_flags = 0;

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
    task->vdso = get_vdso();
    task->current_thread = 0;
    task->main_thread = 0;
    task->pid = get_next_pid();
    task->nice = parent->nice;
    task->current_nice = task->nice;
    task->exit_code = 0;
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
    vmm_copy_stack(task->vmm, parent_thread->altstack_base, (parent_thread->altstack - parent_thread->altstack_base), VMM_USER_BIT | VMM_WRITE_BIT);
    //vdso_remap(task->vmm, task->vdso);
    engrave_vmareas(task, parent);
    kprintf("Process %d duplicated\n", task->pid);
    return task;
}

void alter_process_on_exec(process_t * task, struct loaded_elf * ld, char const ** argv, char const ** envp) {
    process_t saved_task;
    memcpy(&saved_task, task, sizeof(process_t));
    memset(task, 0, sizeof(process_t));

    task->vmm = saved_task.vmm;
    task->vm_areas = 0;

    task->vdso = get_vdso();
    vdso_set_data(task->vdso, VDSO_ENTRY_SIGNAL_TRAMP, signal_trampoline, VDSO_REGION_SIZE(8));
    set_vector_vdso(ld->auxv, (void*)task->vdso);

    memset(task->threads, 0, sizeof(thread_t) * MAX_THREADS);
    task->thread_count = 0;
    task->current_thread = 0;
    task->main_thread = 0;

    task->heap_base = 0;
    task->heap_end = 0;
    task->heap_max_size = 0;

    task->nice = saved_task.nice;
    task->current_nice = task->nice;
    task->exit_code = 0;

    task->sleep_time = 0;
    task->cpu_time = 0;
    task->last_scheduled = 0;
    task->locks = 0;

    task->stack_max_size = PROCESS_STACK_SIZE;

    for (int i = 1; i < NSIG; i++) {
        task->signal_queue[i] = 0;
        memset(&(task->signal_handlers[i]), 0, sizeof(struct sigaction));
    }

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
    
    task->entry_address = ld->entry;
    task->argv = (char**)argv;
    task->envp = (char**)envp;
    task->auxv = ld->auxv;
    task->auxv_size = ld->auxv_size;

    init_thread(task, ld->entry);

    kprintf("Exec: Process %d created\n", task->pid);
}

int exec(process_t * task, char const *path, char const **argv, char const **envp) {

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
    struct loaded_elf * ld = elf_load_elf(task->vmm, buf, size);
    alter_process_on_exec(task, ld, argv, envp);
    kfree(buf);
    return 0;
}

void execve(process_t* task, const char * path, const char ** argv, const char ** envp) {
    exec(task, path, argv, envp);
}

int sched_sigsuspend_check(thread_t * task) {
    if (task->sigsuspend_mask) {
        //Check if there is any pending signal allowed by the sigprocmask
        for (int i = 1; i < NSIG; i++) {
            if (task->process->signal_queue[i] && (task->sigprocmask & (1 << i))) {
                return i; //There is a sigsuspend signal pending, we can schedule
            }
        }

        //Check if there is any signal that will kill the process
        //We don't check the sigprocmask here, as we want to kill the process
        if (task->process->signal_queue[SIGKILL]) {
            task->unsuspend_signal = SIGKILL;
            return SIGKILL; //We can kill the process
        }
        if (task->process->signal_queue[SIGSTOP]) {
            task->unsuspend_signal = SIGSTOP;
            return SIGSTOP; //We can stop the process
        }
    }

    return -1; //Can't resume
}

uint8_t sched_thread(process_t * task) {
    int current_thread_index = task->current_thread + 1;
    if (current_thread_index >= task->thread_count) {
        current_thread_index = 0;
    }

    while (
        task->threads[current_thread_index].status != THREAD_STATUS_READY && 
        task->threads[current_thread_index].status != THREAD_STATUS_RUNNING &&
        task->threads[current_thread_index].waiting != 0
    ) {
        
        if (task->threads[current_thread_index].status == THREAD_STATUS_INTERRUPTIBLE_SLEEP) {
            int signal_check = sched_sigsuspend_check(&(task->threads[current_thread_index]));
            if (signal_check == SIGKILL) {
                task->threads[current_thread_index].status = THREAD_STATUS_ZOMBIE;
                break;
            }
            if (signal_check == SIGSTOP) {
                task->threads[current_thread_index].status = THREAD_STATUS_UNINTERRUPTIBLE_SLEEP;
                break;
            }
            if (signal_check > 0) {
                task->threads[current_thread_index].status = THREAD_STATUS_READY;
                task->threads[current_thread_index].sigprocmask = task->threads[current_thread_index].sigsuspend_mask;
                task->threads[current_thread_index].sigsuspend_mask = 0;
                task->threads[current_thread_index].unsuspend_signal = signal_check;
                break;
            }
        }

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

void move_processes() {
    //Iterate over the process list and remove any NULL process by moving the rest to the left
    for (int i = 0; i < process_count; i++) {
        if (process_list[i].pid == -1) {
            for (int j = i; j < process_count - 1; j++) {
                memcpy(&(process_list[j]), &(process_list[j + 1]), sizeof(process_t));
            }
            process_count--;
            i--;
        }
    }
}

void delete_process(process_t * task) {
    
    if (task->vdso) {
        //vdso_free(task->vdso);
    }
    for (int i = 0; i < task->thread_count; i++) {
        thread_t * thread = &(task->threads[i]);
        if (thread->ustack_base) {
            //stackfree(thread->process->vmm, thread->ustack_base, PROCESS_STACK_SIZE);
        }
        if (thread->kstack_base) {
            //kstackfree(thread->process->vmm, thread->kstack_base, PROCESS_STACK_SIZE);
        }
    }
    memset(task, 0, sizeof(process_t));
    task->pid = -1;
    move_processes();
    kprintf("Process %d deleted\n", task->pid);
}

int16_t waitpid(thread_t * thread, int pid, int * status, int options) {
    if (pid == -1) {
        for (int i = 0; i < process_count; i++) {
            if (process_list[i].ppid == thread->process->pid) {
                if (process_list[i].global_status == PROCESS_STATUS_ZOMBIE) {
                    *status = PROCESS_STATUS_ZOMBIE;
                    delete_process(&(process_list[i]));
                    return process_list[i].pid;
                }
                if (process_list[i].global_status == PROCESS_STATUS_SIGSTOP && (options & WUNTRACED)) {
                    *status = PROCESS_STATUS_SIGSTOP;
                    return process_list[i].pid;
                }
                if (process_list[i].global_status == PROCESS_STATUS_SIGCONT && (options & WCONTINUED)) {
                    *status = PROCESS_STATUS_SIGCONT;
                    return process_list[i].pid;
                }
            }
        }
    } else {
        
        if (pid < 0 || pid >= process_count) {
            return -1;
        }
        process_t * process = get_process_by_pid(pid);
        if (process->ppid == thread->process->pid) {
            if (process->global_status == PROCESS_STATUS_ZOMBIE) {
                *status = PROCESS_STATUS_ZOMBIE;
                delete_process(process);
                return process->pid;
            }
            if (process->global_status == PROCESS_STATUS_SIGSTOP && (options & WUNTRACED)) {
                *status = PROCESS_STATUS_SIGSTOP;
                return process->pid;
            }
            if (process->global_status == PROCESS_STATUS_SIGCONT && (options & WCONTINUED)) {
                *status = PROCESS_STATUS_SIGCONT;
                return process->pid;
            }
        }
    }
    
    //No process has exited
    if (options & WNOHANG) {
        *status = 0;
        return -1;
    }

    //Wait for a process to exit
    thread->waiting = 1;
    sleep(thread, SLEEP_WAITPID);
    sched();
}

process_t * sched() {
    process_t * task = 0x0;
    process_t * processes[PROCESS_PRIORITIES][process_count];
    memset(processes, 0, sizeof(process_t *) * PROCESS_PRIORITIES * process_count);
    int prio_list_size[PROCESS_PRIORITIES] = {0};
    for (int i = 0; i < process_count; i++) {
        process_list[i].current_nice--;
        if (process_list[i].current_nice < 0) {
            process_list[i].current_nice = 0;
        }
        processes[process_list[i].current_nice][prio_list_size[process_list[i].current_nice]++] = &(process_list[i]);
    }

    //Dump the processes array
    for (int i = 0; i < PROCESS_PRIORITIES; i++) {
        if (prio_list_size[i] == 0)
            continue;

        kprintf("PQUEUE %d: ", i);
        for (int j = 0; j < prio_list_size[i]; j++) {
            kprintf("[PID:%d|TSB:%llu|NICE:%d|CNICE:%d] ", processes[i][j]->pid, processes[i][j]->last_scheduled, processes[i][j]->nice, processes[i][j]->current_nice);
        }
        kprintf("\n");
    }

    for (int i = 0; i < PROCESS_PRIORITIES; i++) {
        process_t * candidate = 0x0;
        unsigned long long last_scheduled = 99999999999999999;
        for (int j = 0; j < prio_list_size[i]; j++) {
            if (processes[i][j]->last_scheduled < last_scheduled) {
                last_scheduled = processes[i][j]->last_scheduled;
                candidate = processes[i][j];
            }
        }
        
        if (candidate == 0x0)
            continue;

        if (sched_thread(candidate)) {
            kprintf("Chosen candidate [PID:%d|TSB:%llu|NICE:%d|CNICE:%d]\n", candidate->pid, candidate->last_scheduled, candidate->nice, candidate->current_nice);
            task = candidate;
            break;
        }
    }

    if (task == 0x0)
        panic("No process to schedule\n");

    task->last_scheduled = get_ticks_since_boot();
    task->current_nice = task->nice;
    current_process = task;
    return task;
}

struct sigaction * select_signal(thread_t * thread, int * signo) {
    process_t * process = thread->process;
    sigset_t thread_signal_mask = thread->sigprocmask;
    sigset_t thread_sigsuspend_mask = thread->sigsuspend_mask;
    struct task_signal chosen_signal = {.signo = 0, .next = 0};
    if (thread->unsuspend_signal) {
        if (thread->process->signal_queue[thread->unsuspend_signal] == 0x0)
            panic("Scheduler error: unsuspend signal not set\n");
        if (!dequeue_signal(process->signal_queue, &chosen_signal, thread->unsuspend_signal))
            panic("Scheduler error: unsuspend signal not found\n");
        thread->unsuspend_signal = 0;
        thread->sigprocmask = thread_sigsuspend_mask;
        thread->sigsuspend_mask = 0;
    } else {
        for (int i = 1; i < NSIG; i++) {
            if (process->signal_queue[i] != 0) {
                if ((thread_signal_mask & (1 << i)) == 0) {
                    if (!dequeue_signal(process->signal_queue, &chosen_signal, i))
                        panic("Scheduler error: signal not found\n");
                    break;
                }
            }
        }
    }

    if (chosen_signal.signo != 0x0) {
        *signo = chosen_signal.signo;
        return &(process->signal_handlers[chosen_signal.signo]);
    } else {
        *signo = 0;
        return 0x0;
    }
}

process_t *get_process_by_pid(int pid) {
    for (int i = 0; i < process_count; i++) {
        if (process_list[i].pid == pid) {
            return &(process_list[i]);
        }
    }
    return 0;
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

void thread_exit(thread_t * thread) {
    thread->status = THREAD_STATUS_ZOMBIE;
    sched();
}

process_t * create_user_process(struct page_directory* pd, void * init, char * tty) {
    process_t * task = &(process_list[process_count++]);
    memset(task, 0, sizeof(process_t));

    task->vmm = vmm_copy_kernel(pd);
    task->vm_areas = 0;
    task->vdso = 0;
    
    memset(task->threads, 0, sizeof(thread_t) * MAX_THREADS);
    task->thread_count = 0;
    task->current_thread = 0;
    task->main_thread = 0;
    task->heap_base = 0;
    task->heap_end = 0;
    task->heap_max_size = 0;
    
    task->nice = 10;
    task->current_nice = task->nice;
    task->exit_code = 0;
    task->global_status = 
    task->sleep_time = 0;
    task->cpu_time = 0;
    task->last_scheduled = 0;
    task->locks = 0;

    task->stack_max_size = PROCESS_STACK_SIZE;

    for (int i = 1; i < NSIG; i++) {
        task->signal_queue[i] = 0;
        memset(&(task->signal_handlers[i]), 0, sizeof(struct sigaction));
    }

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
    task->vdso = get_vdso();
    vdso_set_data(task->vdso, VDSO_ENTRY_SIGNAL_TRAMP, signal_trampoline, VDSO_REGION_SIZE(8));

    task->regular_tty = task->open_files[PROCFILE_STDIN];
    task->io_tty = task->open_files[PROCFILE_STDERR];
    task->ctty = &(task->regular_tty);
    
    task->entry_address = init;
    task->auxv = 0x0;
    task->auxv_size = 0x0;
    task->argv = 0x0;
    task->envp = 0x0;

    init_thread(task, init);

    kprintf("Process %d created\n", task->pid);
    return task;
}

void * get_signal_trampoline(process_t * task) {
    void * trampoline = 0;
    vdso_get_data(task->vdso, VDSO_ENTRY_SIGNAL_TRAMP, &trampoline, 0);
    return trampoline;
}

void init_process(const char * _init_path, const char * _idle_path, char * tty) {
    process_t * init_proc = create_user_process(get_pml4(), (void*)_idle, tty);
    init_proc->pid = 0;
    current_process = init_proc;
    const char ** argv = kmalloc(2 * sizeof(char*));
    argv[0] = _init_path;
    argv[1] = 0;
    const char ** envp = kmalloc(2 * sizeof(char*));
    envp[0] = 0;
    
    exec(init_proc, _init_path, argv, envp);

    thread_t * main_thread = &(get_current_process()->threads[0]);
    main_thread->status = THREAD_STATUS_RUNNING;

    struct tss * tss = arch_get_cpu(main_thread->core_id)->tss;
    tss_set_stack(tss, main_thread->kstack, 0);
    tss_set_stack(tss, main_thread->ustack, 3);

#ifdef PREEMPTION_TICKS
    if (PREEMPTION_TICKS != 0) {
        set_preeption_ticks(PREEMPTION_TICKS);
        enable_preemption();
    }
#endif

    disable_debugger();

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
    return current_process;
}

void process_signals(thread_t * thread) {
    process_t * process = thread->process;
    for (int i = 0; i < NSIG; i++) {
        if (process->signal_queue[i] != 0) {
            struct task_signal * signal = process->signal_queue[i];
            process->signal_queue[i] = 0;
            if (process->signal_handlers[i].sa_handler != SIG_IGN) {
                process->signal_handlers[i].sa_handler(signal);
            }
            kfree(signal);
        }
    }
}