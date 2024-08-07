//Early debug interface
//Allows you to write to a device and store messages in a buffer

#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdarg.h>
#include <omen/libraries/std/stddef.h>

#define ERROR(code, str, ...) { kdebug("[ERROR] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__); return code; }

//Macro to print debug messages in a custom format
#define DBG_ERROR(str, ...) kdebug("[ERROR] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__)
#define DBG_WARN(str, ...) kdebug("[WARN] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__)
#define DBG_INFO(str, ...) kdebug("[INFO] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__)
#define DBG_DEBUG(str, ...) kdebug("[DEBUG] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__)

//Setup the debugger over a device
//TODO: Use an abstract device, not a callback!
void init_debugger(const char * device_name);

//Print right away to the debugger
void kprintf(const char * str, ...);

//Write to the debug buffer
void kdebug(const char * str, ...);

//Print the entire debug buffer
void kdump();
#endif