#include <stdarg.h>

#include <generic/config.h>
#include <omen/apps/debug/debug.h>
#include <omen/libraries/std/stdio.h>
#include <omen/libraries/std/stdint.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/std/stddef.h>
#include <omen/libraries/std/stdbool.h>
#include <omen/managers/cpu/process.h>
#include <omen/managers/dev/devices.h>

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

void kprintf(const char * str, ...) {
    if (writer == NULL || !debug_enabled) {
        return;
    }
    
    va_list args;
    va_start(args, str);
    memset(debug_buffer, 0, DEBUG_MESSAGE_BUFFER);
    vsnprintf(debug_buffer, DEBUG_MESSAGE_BUFFER, str, args);
    device_write(writer->name, strlen(debug_buffer), 0, (uint8_t*)debug_buffer);
    va_end(args);
}

char * get_debug_device_name() {
    if (writer == NULL) {
        return NULL;
    }
    return writer->name;
}

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 */
 char* itoa(int64_t value, int base) {
    // check that the base if valid
    for (int i = 0; i < DEBUG_MESSAGE_BUFFER; i++) {
        debug_buffer[i] = 0;
    }
    if (base < 2 || base > 36) { *debug_buffer = '\0'; return debug_buffer; }

    char* ptr = debug_buffer, *ptr1 = debug_buffer, tmp_char;
    int64_t tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return debug_buffer;
}

//atoi
int64_t atoi(const char * str) {
    int64_t res = 0; // Initialize result

    // Iterate through all characters of input string and
    // update result
    for (int i = 0; str[i] != '\0'; ++i)
        res = res*10 + str[i] - '0';

    // return result.
    return res;
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
