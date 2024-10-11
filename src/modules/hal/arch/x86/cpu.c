#include <omen/hal/arch/x86/cpu.h>
#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/libraries/std/stdint.h>
#include <omen/hal/arch/x86/tss.h>
#include <omen/hal/arch/x86/syscall.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/hal/arch/x86/gdt.h>
#include <omen/hal/arch/x86/int.h>
#include <omen/apps/debug/debug.h>

#define SIMD_CONTEXT_SIZE 512
#define CR0_MONITOR_COPROC (1 << 1)
#define CR0_EM (1 << 2)
#define CR0_NUMERIC_ERROR (1 << 5)
#define CR4_FXSR (1 << 9)
#define CR4_SIMD_EXCEPTION (1 << 10)

#define FPU_ENABLE_CODE 0x200
#define FPU_CONTROL_WORD 0x37F

extern void reloadGsFs();
extern void setGsBase(uint64_t base);
extern void setKernelGsBase(uint64_t base);
extern void _swapgs();

boot_smp_info_t ** cpus;
uint64_t cpu_count;
uint32_t bsp_lapic_id;
cpu_context_t cpu[MAX_CPUS];

void callback(boot_smp_info_t *lcpu) {
    (void)lcpu;
    kprintf("CPU %d is halting\n", lcpu->processor_id);
    while (1) {
       __asm__("hlt");
    }
}

void startup_tss(struct tss ** tss, uint8_t cpuid, void * kstack) {
    *tss = get_tss(cpuid);
    void * ist0 = kstackalloc(KERNEL_STACK_SIZE) + KERNEL_STACK_SIZE;
    memset(ist0 - KERNEL_STACK_SIZE, 0, KERNEL_STACK_SIZE);
    void * ist1 = kstackalloc(KERNEL_STACK_SIZE) + KERNEL_STACK_SIZE;
    memset(ist1 - KERNEL_STACK_SIZE, 0, KERNEL_STACK_SIZE);

    tss_set_stack(*tss, kstack, 0);
    tss_set_ist(*tss, 0, (uint64_t)ist0);
    tss_set_ist(*tss, 1, (uint64_t)ist1);
}

void startup_cpu(uint8_t cpuid) {
    cpu_context_t * ctx = &cpu[cpuid];
    kprintf("Starting CPU %d\n", ctx->cid);
    ctx->ustack = 0;
    ctx->cinfo = kmalloc(sizeof(struct cpu_context_info));
    memset(ctx->cinfo, 0, sizeof(struct cpu_context_info));
    ctx->cinfo->cs = GDT_KERNEL_CODE_ENTRY * sizeof(gdt_entry_t);
    ctx->cinfo->ss = GDT_KERNEL_DATA_ENTRY * sizeof(gdt_entry_t);
    ctx->cinfo->stack = (uint64_t)kstackalloc(KERNEL_STACK_SIZE) + KERNEL_STACK_SIZE;
    memset(ctx->cinfo->stack - KERNEL_STACK_SIZE, 0, KERNEL_STACK_SIZE);
    ctx->cinfo->thread = 0;

    startup_tss(&(ctx->tss), cpuid, ctx->cinfo->stack);

    load_gdt(cpuid);
    reloadGsFs();
    setGsBase((uint64_t)ctx);
    setKernelGsBase((uint64_t)ctx);

    syscall_enable(GDT_KERNEL_CODE_ENTRY * sizeof(gdt_entry_t), GDT_USER_CODE_ENTRY * sizeof(gdt_entry_t));
    load_interrupts_for_local_cpu();
}

cpu_context_t * arch_get_cpu(uint8_t cpuid) {
    return &cpu[cpuid];
}

cpu_context_t * arch_get_bsp_cpu() {
    return &cpu[bsp_lapic_id];
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
        if (cpu[i].cid != bsp_lapic_id)
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