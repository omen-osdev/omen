#ifndef _DEBUG_H
#define _DEBUG_H

#include <omen/libraries/std/stdarg.h>
#include <omen/libraries/std/stddef.h>

#define ERROR(code, str, ...) { kdebug("[ERROR] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__); return code; }

#define DBG_ERROR(str, ...) kdebug("[ERROR] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__)
#define DBG_WARN(str, ...) kdebug("[WARN] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__)
#define DBG_INFO(str, ...) kdebug("[INFO] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__)
#define DBG_DEBUG(str, ...) kdebug("[DEBUG] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__)

void init_debugger(void * (*writer)(const char * str, size_t len));
void kprintf(const char * str, ...);
void kdebug(const char * str, ...);
void kdump();
#endif