#include <omen/managers/cpu/process.h>
#include <omen/libraries/std/stddef.h>

static process_t current_process;

process_t get_current_process() {
    return current_process;
}


void init_process() {
    //This is a debug processs
    current_process = (process_t) {
        .context = NULL,
        .cpu = NULL,
        .vm = NULL,
        .status = 0,
        .privilege = 0,
        .signal_pending = 0,
        //TODO: Implement signal queue
        //.signal_queue = NULL,
        //.signal_handlers = {0},
        .nice = 0,
        .current_nice = 0,
        //.frame = NULL,
        .heap = NULL,
        .sleep_time = 0,
        .cpu_time = 0,
        .last_scheduled = 0,
        .exit_code = 0,
        .exit_signal = 0,
        .pdeath_signal = 0,
        .pid = 0,
        .ppid = 0,
        .parent = NULL,
        .uid = 0,
        .gid = 0,
        .regular_tty = "",
        .io_tty = "",
        .tty = NULL,
        .locks = 0,
        .open_files = NULL,
        .entry_address = NULL,
        .descriptors = NULL,
        .next = NULL,
        .prev = NULL
    };
}

char * get_current_tty() {
    return current_process.tty;
}

void set_current_tty(char * tty) {
    current_process.tty = tty;
}