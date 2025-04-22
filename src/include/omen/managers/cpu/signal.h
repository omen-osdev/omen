#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <omen/managers/cpu/process.h>
#include <omen/libraries/std/stdint.h>

typedef struct task_signal {
    int signal;
    void * signal_data;
    uint64_t signal_data_size;
    struct task_signal *next;
} signal_t;

//Define type for signal handler
void subscribe_signal(process_t * task, int signal, sighandler_t handler);
void add_signal(process_t *task, ,int signal, void * data, uint64_t size);
void process_signals(process_t *task);
#endif