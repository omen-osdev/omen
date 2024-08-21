#include <stdarg.h>

#include <generic/config.h>
#include <omen/apps/debug/debug.h>
#include <omen/libraries/std/stdio.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/std/stddef.h>
#include <omen/libraries/std/stdbool.h>
#include <omen/managers/dev/devices.h>
#include <omen/managers/fb/gop.h>

struct device * writer = NULL;
char debug_buffer[DEBUG_MESSAGE_BUFFER];

bool debug_enabled = false;

void init_debugger(const char * device_name) {
    if (device_name == NULL) {
        return;
    }
    if (strlen(device_name) > DEVICE_NAME_MAX_SIZE) {
        return;
    }
    struct device * dev = device_search(device_name);
    if (dev == NULL) {
        return;
    }
    writer = dev;
    debug_enabled = true;
}

void printstr(char* str) {

	while(*str) {
		gop_putchar(*str);
		str++;
	}
}

void kprintf(const char * str, ...) {

	/*
    if (writer == NULL || !debug_enabled) {
        return;
    }
    */

    va_list args;
    va_start(args, str);
    memset(debug_buffer, 0, DEBUG_MESSAGE_BUFFER);
    vsnprintf(debug_buffer, DEBUG_MESSAGE_BUFFER, str, args);
    printstr(debug_buffer);
    //device_write(writer->name, strlen(debug_buffer), 0, (uint8_t*)debug_buffer);
    //TODO I just pass the buffer to the printstr function which uses the gop fb. Someone may want to make the
    //device_write function work with the gop_putchar when applicable
    va_end(args);
}

void kdebug(const char * str, ...) {
    if (writer == NULL || !debug_enabled) {
        return;
    }

    //call kprintf internally
    va_list args;
    va_start(args, str);
    kprintf(str, args);
    va_end(args);
}
