#include <omen/apps/debug/debugger.h>
#include <omen/libraries/std/stdio.h>
#include <omen/libraries/std/stdarg.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/std/stddef.h>
#include <omen/libraries/basic/circlist.h>
#include <omen/libraries/std/stdbool.h>

static void * (*writer)(const char * str, size_t len);

char debug_buffer[DEBUG_MESSAGE_BUFFER] = {0};
bool debug_enabled = false;

void init_debugger(void * (*writer)(const char * str, size_t len)) {
    writer = writer;

    if (circlist_init(&debug_buffer, DEBUG_MESSAGE_BUFFER) != SUCCESS) {
        kprintf("Failed to initialize debug buffer\n");
    } else {
        debug_enabled = true;
    }
}

void kprintf(const char * str, ...) {
    if (writer == NULL || !debug_enabled) {
        return;
    }

    va_list args;
    va_start(args, str);
    vprintf(str, args);
    va_end(args);
}

void kdebug(const char * str, ...) {
    if (!debug_enabled) {
        return;
    }

    va_list args;
    va_start(args, str);
    char buffer[DEBUG_MESSAGE_BUFFER] = {0};
    vsnprintf(buffer, DEBUG_MESSAGE_BUFFER, str, args);
    va_end(args);

    if (buffer[strlen(buffer) - 1] != '\n') {
        buffer[strlen(buffer)] = '\n';
    }

    if (circlist_write(&debug_buffer, buffer, strlen(buffer)) != SUCCESS) {
        kprintf("Failed to write to debug buffer\n");
    }
}

void kdump() {
    if (!debug_enabled || writer == NULL) {
        return;
    }

    uint64_t used = circlist_used_space(&debug_buffer);
    char buffer[used] = {0};
    if (circlist_read(&debug_buffer, buffer, used) != SUCCESS) {
        kprintf("Failed to read from debug buffer\n");
    }

    writer(buffer, used);
}