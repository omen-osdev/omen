#ifndef _ERROR_H
#define _ERROR_H

#include <stdarg.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>

#define RECOVERABLE_ERROR(code, str, ...) { kdebug("[ERROR] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__); return code; }
#define UNRECOVERABLE_ERROR(str, ...) { kdebug("[ERROR] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__); panic("Unrecoverable error"); }

#endif