#ifndef _STDDEF_H
#define _STDDEF_H

#include <omen/libraries/std/stdint.h>

#define NULL ((void*)0)

typedef unsigned long size_t;
typedef int64_t status_t;

#define SUCCESS 0
#define FAILURE -1
#define INVALID_ARGUMENT -2
#define NOT_FOUND -3
#define ALREADY_EXISTS -4
#define OUT_OF_MEMORY -5
#define NOT_IMPLEMENTED -6

#endif