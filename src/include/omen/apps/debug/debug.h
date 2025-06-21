//Early debug interface
//Allows you to write to a device and store messages in a buffer

#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdarg.h>
#include <omen/libraries/std/stddef.h>
#include <omen/libraries/std/stdint.h>

#define DEBUG_LEVEL_NONE 0
#define DEBUG_LEVEL_ERROR 1
#define DEBUG_LEVEL_WARN 2
#define DEBUG_LEVEL_STRACE 3
#define DEBUG_LEVEL_INFO 4
#define DEBUG_LEVEL_DEBUG 5

#define ERROR(code, str, ...) {kprintf_error("[ERROR] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__); return code; }

//Macro to print debug messages in a custom format
#define DBG_ERROR(str, ...) kprintf_error("[ERROR] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__)
#define DBG_WARN(str, ...) kprintf_warning("[WARN] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__)
#define DBG_STRACE(str, ...) kprintf_strace("[STRACE] " str, ##__VA_ARGS__)
#define DBG_INFO(str, ...) kprintf_info("[INFO] " str, ##__VA_ARGS__)
#define DBG_DEBUG(str, ...) kprintf_debug("[DEBUG] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__)

//Setup the debugger over a device
//TODO: Use an abstract device, not a callback!
#define BREAKPOINT() __breakpoint()
void __breakpoint();
void init_debugger(const char * device_name);
void disable_debugger();
void enable_debugger();
uint8_t is_debugger_enabled();
char * get_debug_device_name();
void set_debug_level(uint8_t level);

int64_t atoi(const char * str);

char* itoa(int64_t value, int base);

//Print right away to the debugger
void kprintf(const char * str, ...);
void kprintf_error(const char * str, ...);
void kprintf_warning(const char * str, ...);
void kprintf_strace(const char * str, ...);
void kprintf_info(const char * str, ...);
void kprintf_debug(const char * str, ...);

//Print the entire debug buffer
void kdump();
#endif