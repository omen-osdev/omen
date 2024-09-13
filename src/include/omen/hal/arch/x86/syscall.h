#ifndef _SYSCALL_H
#define _SYSCALL_H
#include <omen/libraries/std/stdint.h>
#include <omen/hal/arch/x86/cpu.h>
#include <omen/managers/cpu/process.h>

#define SYSCALL_INITIAL_FLAGS 0x200

#define SYSCALL_SUCCESS (0)
#define SYSCALL_UNDEFINED (-1)
#define SYSCALL_ERROR (-2)

#define SYSCALL_HANDLER_COUNT 256

typedef void (*syscall_handler)(process_t*caller_task, cpu_context_t* ctx);
extern void syscall_enable(uint16_t kernel_segment, uint16_t user_segment);
void global_syscall_handler(cpu_context_t* ctx);
void syscall_set_user_gs(uint64_t addr);
void syscall_set_kernel_gs(uintptr_t addr);
#endif