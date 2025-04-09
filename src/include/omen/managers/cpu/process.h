#ifndef _PROCESS_H
#define _PROCESS_H

#include <generic/config.h>
#include <omen/hal/hal.h>
#include <omen/managers/io/signal/signal.h>
#include <omen/libraries/std/stdint.h>
#include <omen/managers/mem/vmm.h>
#include <omen/libraries/allocators/heap_allocator.h>

#define PROCESS_STATUS_READY 0
#define PROCESS_STATUS_RUNNING 1
#define PROCESS_STATUS_SLEEPING 2
#define PROCESS_STATUS_WAITING 3
#define PROCESS_STATUS_ZOMBIE 4
#define PROCESS_STATUS_STOPPED 5
#define PROCESS_STATUS_DEAD 6

#define MAX_OPEN_FILES 32

#define VMAREA_EXT_COW      0x1
#define VMAREA_EXT_SHARED   0x2
#define VMAREA_EXT_GUARD    0x4

typedef int process_status_t;

struct vm_area {
    void * start;
    void * end;
    uint8_t flags;
    uint8_t extended_flags;
    uint64_t page_size;
    struct vm_area * next;
};

typedef struct process {
    cpu_context_t *context;
    void * ustack;
    void * ustack_base;
    void * kstack;
    void * kstack_base;
    uint64_t ustack_max_size;
    void * heap_base;
    void * heap_end;
    void * heap_max_size;
    struct vm_area *vm_areas;
    process_status_t status;
    uint8_t core_id;
    uint8_t privilege;
    int signal_pending;
    //signal_queue_t *signal_queue;
    //signal_handler_t signal_handlers[PROCESS_SIGNAL_MAX];

    long nice;
    long current_nice;
    //int_error_frame_t *frame;

    unsigned long long sleep_time;
    unsigned long long cpu_time;
    unsigned long long last_scheduled;


    int exit_code, exit_signal;
    int pdeath_signal;

    int16_t pid;
    int16_t ppid;

    struct process *parent;

    int16_t uid;
    int16_t gid;

    char regular_tty[32];
    char io_tty[32];
    char *tty;

    unsigned int locks;

    int open_files[MAX_OPEN_FILES];
    int open_files_count;

    char fxsave_region[512] __attribute__((aligned(16)));

    void * entry_address;
    struct process *next, *prev;

} process_t;


void init_process(const char * init_path, const char * idle_path, char * tty);
void create_vmarea(process_t* process, void * start, void * end, uint8_t flags, uint8_t extended_flags, uint64_t page_size);
struct vm_area* is_in_vmarea(process_t* process, void * address);
void * find_shm_vmarea(process_t * task, void * hint, uint64_t size);
void duplicate_vmarea_cow(process_t * task, struct vm_area* vma);
void returnoexit();
int16_t fork();
process_t * sched();
void execve(const char * path, const char * argv, const char * envp);
int exec(char const *path);
void exit(int error_code);
process_t * create_user_process(void * init, char* tty);
process_t * get_current_process();
char * get_current_tty();
void set_current_tty(char * tty);
#endif