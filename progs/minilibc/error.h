#ifndef _ERROR_H
#define _ERROR_H

#include <stdarg.h>
#include "stdio.h"

#define RECOVERABLE_ERROR(code, str, ...) { printf("[ERROR] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__); return code; }
#define UNRECOVERABLE_ERROR(str, ...) { printf("[ERROR] %s:%d: " str, __FILE__, __LINE__, ##__VA_ARGS__); panic("Unrecoverable error"); }

#endif