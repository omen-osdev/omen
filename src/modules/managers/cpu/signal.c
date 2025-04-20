#include <omen/managers/cpu/signal.h>
#include <omen/libraries/std/stdint.h>
#include <omen/apps/panic/panic.h>

void subscribe_signal(process_t * task, int signal, sighandler_t handler) {
    if (task == 0) panic("No such task\n");
    if (signal > TASK_SIGNAL_MAX || signal < 0) panic("Signal out of bounds\n");
    get_current_process()->signal_handlers[signal] = handler;
}

void add_signal(process_t * task, int signal, void * data, uint64_t size) {
    if (task == 0) {
        panic("No such task\n");
        return;
    }

    //Add signal to signal_queue linked list
    signal_t * signal_struct = kmalloc(sizeof(struct task_signal));
    signal_struct->signal = signal;
    signal_struct->next = 0;
    signal_struct->signal_data = data;
    signal_struct->signal_data_size = size;

    if (task->signal_queue == 0) {
        task->signal_queue = signal_struct;
    } else {
        signal_t * current = task->signal_queue;
        while (current->next != 0) {
            current = current->next;
        }
        current->next = signal_struct;
    }
}

void __attribute__((noinline)) process_signals(process_t * task) {
    //Process one signal in the queue
    if (task->signal_queue != 0) {
        signal_t * current = task->signal_queue;
        if (current->next != 0) {
            task->signal_queue = current->next;
        } else {
            task->signal_queue = 0;
        }

        sighandler_t handler = task->signal_handlers[current->signal];
        kfree(current);

        if (handler != 0) {
            handler(current->signal, current->signal_data, current->signal_data_size);
        }
    }
}