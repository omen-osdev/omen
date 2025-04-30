#ifndef _SLINE_H
#define _SLINE_H

#define SQUEUE_MAX 64

#include <omen/managers/cpu/process.h>
#include <omen/libraries/std/time.h>
void sleep(thread_t * thread, int condition);
void wakeup(int condition);
void force_wakeup_task(thread_t * thread);
int nanosleep(thread_t * thread, struct timespec *duration, struct timespec *rem);
#endif