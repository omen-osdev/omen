#include <omen/hal/arch/x86/cpu.h>
#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/libraries/std/stdint.h>
#include <omen/apps/debug/debug.h>

#define SIMD_CONTEXT_SIZE 512
#define CR0_MONITOR_COPROC (1 << 1)
#define CR0_EM (1 << 2)
#define CR0_NUMERIC_ERROR (1 << 5)
#define CR4_FXSR (1 << 9)
#define CR4_SIMD_EXCEPTION (1 << 10)

#define FPU_ENABLE_CODE 0x200
#define FPU_CONTROL_WORD 0x37F

boot_smp_info_t ** cpus;
uint64_t cpu_count;
uint32_t bsp_lapic_id;
cpu_t cpu[MAX_CPUS];

void callback(boot_smp_info_t *lcpu) {
    (void)lcpu;
    while (1) {
        kprintf("CPU %d is halting\n", lcpu->processor_id);
        while (!cpu[lcpu->processor_id].ready) {
        }
    }
}

void arch_set_alive(uint8_t cpuid, uint8_t alive) {
    cpu[cpuid].ready = alive;
}

void startup_cpu(uint8_t cpuid) {
    cpu_t * lcpu = &cpu[cpuid];
    if(!lcpu->ready) lcpu->ready = 1;
}

void arch_init_cpu() {
    bsp_lapic_id = get_smp_bsp_lapic_id();
    cpu_count = get_smp_cpu_count();
    if (cpu_count > MAX_CPUS) {
        cpu_count = MAX_CPUS;
    }
    cpus = get_smp_cpus();

    for (uint64_t i = 0; i < cpu_count; i++) {
        cpu[i].cid = cpus[i]->lapic_id;
        cpus[i]->goto_address = callback;
    }

    startup_cpu(bsp_lapic_id);
}

void arch_init_fpu() {
    uint64_t cr0;
    uint16_t control_word = FPU_CONTROL_WORD;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= FPU_ENABLE_CODE;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
    __asm__ volatile("fldcw %0" : : "m"(control_word));
}

void arch_init_simd() {
    uint64_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~((uint64_t)CR0_EM);
    cr0 |= CR0_MONITOR_COPROC;
    cr0 |= CR0_NUMERIC_ERROR;
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));

    uint64_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_FXSR;
    cr4 |= CR4_SIMD_EXCEPTION;
    __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));
    __asm__ volatile("finit");
}

void arch_simd_save_context(void* ctx) {
    __asm__ volatile("fxsave (%0) "::"r"(ctx));
}

void arch_simd_restore_context(void* ctx) {
    __asm__ volatile("fxrstor (%0) "::"r"(ctx));
}