#ifndef _SLINE_H
#define _SLINE_H

#define SQUEUE_MAX 64

#define TTY_IO_SLINE 0x2

#define SLEEP_WAITPID 0x1

#include <omen/managers/cpu/process.h>
#include <omen/libraries/std/time.h>
void sleep_current(int condition);
void sleep(thread_t * thread, int condition);
void wakeup(int condition);
void force_wakeup_task(thread_t * thread);
int nanosleep(thread_t * thread, struct timespec *duration, struct timespec *rem);
#endif