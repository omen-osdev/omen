#ifndef _PROCESS_H
#define _PROCESS_H

#include <generic/config.h>
#include <omen/hal/hal.h>
#include <omen/managers/io/signal/signal.h>
#include <omen/libraries/std/stdint.h>

struct descriptors {
    uint8_t stdin;
    uint8_t stdout;
    uint8_t stderr;
};

typedef int process_status_t;

typedef struct process {
    cpu_context_t *context;
    cpu_t *cpu;
    struct page_directory* vm;
    process_status_t status;

    uint8_t privilege;
    int signal_pending;
    //signal_queue_t *signal_queue;
    //signal_handler_t signal_handlers[PROCESS_SIGNAL_MAX];

    long nice;
    long current_nice;
    //int_error_frame_t *frame;

    void * heap_address;

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

    void * entry_address;
    struct descriptors* descriptors;
    struct process *next, *prev;

} process_t;

char * get_current_tty();
void set_current_tty(char * tty);
#endif