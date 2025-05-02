#ifndef _SYSCALL_H
#define _SYSCALL_H
#include <omen/libraries/std/stdint.h>
#include <omen/hal/arch/x86/cpu.h>
#include <omen/managers/cpu/process.h>

#define SYSCALL_INITIAL_FLAGS 0x200

#define SYSCALL_SUCCESS (0)
#define SYSCALL_ERROR (-1)
#define SYSCALL_UNDEFINED (-2)

#define SYSCALL_HANDLER_COUNT 512

typedef int64_t (*syscall_handler)(thread_t*caller_thread, cpu_context_t* ctx);
extern void syscall_enable(uint16_t kernel_segment, uint16_t user_segment);
void global_syscall_handler(cpu_context_t* ctx);
#endif