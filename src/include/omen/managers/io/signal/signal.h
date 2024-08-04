#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <omen/libraries/std/stdint.h>

typedef struct task_signal {
    int signal;
    void * signal_data;
    uint64_t signal_data_size;
    struct task_signal *next;
} signal_queue_t;

typedef void (*signal_handler_t)(int, void*, uint64_t);