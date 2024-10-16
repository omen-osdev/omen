#ifndef _PROCESS_H
#define _PROCESS_H

#include <generic/config.h>
#include <omen/hal/hal.h>
#include <omen/managers/io/signal/signal.h>
#include <omen/libraries/std/stdint.h>
#include <omen/managers/mem/vmm.h>

#define PROCESS_STATUS_READY 0
#define PROCESS_STATUS_RUNNING 1
#define PROCESS_STATUS_SLEEPING 2
#define PROCESS_STATUS_WAITING 3
#define PROCESS_STATUS_ZOMBIE 4
#define PROCESS_STATUS_STOPPED 5
#define PROCESS_STATUS_DEAD 6

struct descriptors {
    uint8_t stdin;
    uint8_t stdout;
    uint8_t stderr;
};

typedef int process_status_t;

typedef struct process {
    context_t *context;
    cpu_context_t *cpu;
    struct page_directory* vm;
    process_status_t status;

    uint8_t privilege;
    int signal_pending;
    //signal_queue_t *signal_queue;
    //signal_handler_t signal_handlers[PROCESS_SIGNAL_MAX];

    long nice;
    long current_nice;
    //int_error_frame_t *frame;

    void * heap;

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

    int * open_files;

    char fxsave_region[512] __attribute__((aligned(16)));

    void * entry_address;
    struct descriptors* descriptors;
    struct process *next, *prev;

} process_t;

void init_process(uint64_t addr, uint64_t size);
void returnoexit();
int16_t fork();
process_t * sched();
void execve(const char * path, const char * argv, const char * envp);
void exit(int error_code);
process_t * create_user_process(void * init);
process_t * get_current_process();
char * get_current_tty();
void set_current_tty(char * tty);
#endif