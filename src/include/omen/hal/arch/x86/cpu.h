#ifndef _X86_CPU_H
#define _X86_CPU_H

#include <omen/libraries/std/stdint.h>
#include <omen/hal/arch/x86/tss.h>

struct cpu_context_info {
    uint64_t stack;
    uint64_t cs;
    uint64_t ss;
    uint64_t thread;
}__attribute__((packed));

typedef struct context {
    uint64_t cr3;

    struct cpu_context_info* info;
    
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;  
    uint64_t rdi;
    uint64_t rbp;

    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    uint64_t interrupt_number; 
    uint64_t error_code; 
    
    uint64_t rip; 
    uint64_t cs; 
    uint64_t rflags; 
    uint64_t rsp; 
    uint64_t ss;
} __attribute__((packed)) context_t;

typedef struct cpu {
    uint64_t cid;
    struct cpu_context_info *cinfo;
    void* ustack;
    struct tss *tss;
} cpu_context_t;

cpu_context_t * arch_get_cpu(uint8_t cpuid);
cpu_context_t * arch_get_bsp_cpu();

void arch_init_cpu();
void arch_init_fpu();
void arch_init_simd();
void arch_simd_save_context(void* ctx);
void arch_simd_restore_context(void* ctx);
#endif