#ifndef _MSR_H
#define _MSR_H

#define MSR_APIC                0x1B
#define MSR_EFER                0xC0000080
#define MSR_STAR                0xC0000081
#define MSR_LSTAR               0xC0000082
#define MSR_COMPAT_STAR         0xC0000083
#define MSR_SYSCALL_FLAG_MASK   0xC0000084
#define MSR_FS_BASE             0xC0000100
#define MSR_GS_BASE             0xC0000101
#define MSR_KERN_GS_BASE        0xc0000102

#include <omen/libraries/std/stdint.h>
void cpuGetMSR(uint32_t msr, uint32_t *lo, uint32_t *hi);
void cpuSetMSR(uint32_t msr, uint32_t lo, uint32_t hi);
uint64_t cpu_get_msr_u64(uint32_t msr);
void cpu_set_msr_u64(uint32_t msr, uint64_t value);
#endif