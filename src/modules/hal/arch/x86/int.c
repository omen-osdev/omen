#include <omen/hal/arch/x86/int.h>
#include <omen/hal/arch/x86/io.h>
#include <omen/hal/arch/x86/apic.h>
#include <omen/hal/arch/x86/getcpuid.h>
#include <omen/hal/arch/x86/cpu.h>
#include <omen/hal/arch/x86/gdt.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/managers/mem/vmm.h>
#include <omen/managers/cpu/process.h>
#include <omen/managers/cpu/context.h>
#include <omen/managers/cpu/vmarea.h>
#include <omen/managers/dev/pit.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/allocators/heap_allocator.h>

#define __UNDEFINED_HANDLER __asm__("cli"); kprintf(__func__); (void)frame; panic("Undefined interrupt handler");
#define IS_EXCEPTION(ctx)(ctx->interrupt_number < 32)
#define IS_EXCEPTION_INUM(inum)(inum < 32)
extern void setFsBase(uint64_t base);
extern void* interrupt_vector[IDT_ENTRY_COUNT];
char io_tty[32] = "default\0";
char saved_tty[32];
uint8_t interrupts_ready = 0;
struct idtr idtr;
volatile int dynamic_interrupt = -1;

struct stackFrame {
    struct stackFrame * rbp;
    uint64_t rip;
};

void set_offset(struct idtdescentry* entry, uint64_t offset) {
    entry->offset0 = (uint16_t) (offset & 0x000000000000ffff);
    entry->offset1 = (uint16_t) ((offset & 0x00000000ffff0000) >> 16);
    entry->offset2 = (uint32_t) ((offset & 0xffffffff00000000) >> 32);
}

uint64_t get_offset(struct idtdescentry* entry) {
    uint64_t offset = 0;
    offset |= (uint64_t) entry->offset0;
    offset |= (uint64_t) entry->offset1 << 16;
    offset |= (uint64_t) entry->offset2 << 32;
    return offset;
}

void (*dynamic_interrupt_handlers[256])(cpu_context_t* ctx, uint8_t cpuid) = {0};

void PageFault_Handler(cpu_context_t* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    uint64_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));
    kprintf("Page Fault Address: %lx\n", (uint64_t)faulting_address);
    kprintf("Error code: %lx\n", ctx->error_code);
    thread_t * thread = get_current_thread();
    if (!thread) panic("Page fault, no task detected!\n");
    thread->user_context->cpu_context->cr3 = to_identity_map(ctx->cr3);

    struct vm_area* vma = is_in_vmarea(thread->process, (void*)faulting_address);
    if (!thread->process) {
        kprintf("Page fault, no process found for address %lx\n", faulting_address);
        kprintf("Thread TID: %d\n", thread->id);
        panic("Page fault, no process found!\n");
    }

    if (!vma) {
        kprintf("Page fault, no VMA found for address %lx\n", faulting_address);
        kprintf("Process PID: %d, TID: %d\n", thread->process->pid, thread->id);
        dump_vmareas(thread->process);
        panic("Page fault, no VMA found!\n");
    }

    if ((vma->flags & VMM_WRITE_BIT) && (vma->extended_flags & VMAREA_EXT_SHARED)) {
        kprintf("Page fault in shared area, allowing write and requesting sync\n");
        vma->extended_flags |= VMAREA_EXT_REQ_SYNC;
        mprotect(thread->process->vmm, (void*)faulting_address, vma->page_size, vma->flags);
        thread->user_context->cpu_context->cr3 = from_identity_map(thread->user_context->cpu_context->cr3);
        return;
    }
    if ((vma->flags & VMM_WRITE_BIT) && (vma->extended_flags & VMAREA_EXT_COW)) {
        kprintf("Page fault in COW area, duplicating page\n");
        duplicate_vmarea_cow(thread->process, vma);
        thread->user_context->cpu_context->cr3 = from_identity_map(thread->user_context->cpu_context->cr3);
        kprintf("Page fault in COW area, page duplicated\n");
        return;
    }
    if ((vma->flags & VMM_USER_BIT) && (vma->extended_flags & VMAREA_EXT_STACK_GUARD)) {
        kprintf("Page fault: STACK GUARD\n");
        struct stack stack;
        stack.base = thread->ustack_base;
        stack.top = thread->ustack;
        stack.size = thread->ustack_size;
        stack.guard_size = thread->ustack_guard_size;
        stack.flags = 0;
        grow_stack(thread->process->vmm, &stack);

        remove_vmarea(thread->process, thread->ustack_base-thread->ustack_guard_size);
        remove_vmarea(thread->process, thread->ustack_base);
        thread->ustack_base = stack.base;
        thread->ustack_size = stack.size;
        create_vmarea(thread->process, thread->ustack_base, thread->ustack_base+thread->ustack_size, VMM_USER_BIT | VMM_WRITE_BIT, 0, PAGE_SIZE_4KIB, -1, 0);
        create_vmarea(thread->process, thread->ustack_base-thread->ustack_guard_size, thread->ustack_base, VMM_USER_BIT, VMAREA_EXT_STACK_GUARD, PAGE_SIZE_4KIB, -1, 0);
        thread->user_context->cpu_context->cr3 = from_identity_map(thread->user_context->cpu_context->cr3);
        return;
    }

    panic("Page fault, wtffff\n");
}

void DoubleFault_Handler(cpu_context_t* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    panic("Double fault\n");
}

void GPFault_Handler(cpu_context_t* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    panic("General protection fault\n");
}

void PCI_Handler(cpu_context_t* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    panic("PCI_Handler Not implemented\n");
}

void Syscall_Handler(cpu_context_t* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    panic("Syscall_Handler Not implemented\n");
}

//you may need save_all here
void PitInt_Handler(cpu_context_t* ctx, uint8_t cpuid) {
    tick();
    if (requires_preemption()) {
        sched();
    }
}

void Serial1Int_Handler(cpu_context_t* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    char c = inb(0x3f8);
    kprintf("Serial1: ");
    kprintf("%x", c);
    kprintf("\n");
}

void Serial2Int_Handler(cpu_context_t* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    char c = inb(0x3f8);
    kprintf("Serial2: ");
    kprintf("%x", c);
    kprintf("\n");
}

static void interrupt_exception_handler(cpu_context_t* ctx, uint8_t cpu_id) {
    kprintf("GENERIC EXCEPTION %d ON CPU %d\n", ctx->interrupt_number, cpu_id);
    panic("Exception\n");
}

struct idtdescentry * get_idt_gate(uint8_t entry_offset) {
    return (struct idtdescentry*)(idtr.offset + (entry_offset * sizeof(struct idtdescentry)));
}

void set_io_tty(const char * tty) {
    memset(io_tty, 0, 32);
    strcpy(io_tty, tty);
}

void set_idt_gate(uint64_t handler, uint8_t entry_offset, uint8_t type_attr, uint8_t ist, uint16_t selector) {
    struct idtdescentry* interrupt = (struct idtdescentry*)(idtr.offset + (entry_offset * sizeof(struct idtdescentry)));
    set_offset(interrupt, handler);
    interrupt->type_attr.raw = type_attr;
    interrupt->selector = selector;
    interrupt->ist = ist;
}

void hook_interrupt(uint8_t interrupt, void* handler) {
    if (!interrupts_ready) panic("Interrupts not ready\n");
    dynamic_interrupt_handlers[interrupt] = handler;
}

void unhook_interrupt(uint8_t interrupt) {
    if (!interrupts_ready) panic("Interrupts not ready\n");
    dynamic_interrupt_handlers[interrupt] = (void*)interrupt_exception_handler;
}

void load_interrupts_for_local_cpu() {
    if (interrupts_ready) {
        __asm__("lidt %0" : : "m"(idtr));
    } else {
        panic("Interrupts not ready\n");
    }
}

void init_interrupts() {
    kprintf("### INTERRUPTS STARTUP ###\n");
    
    if (!check_apic()) {
        panic("APIC not found\n");
    }

    idtr.limit = 256 * sizeof(struct idtdescentry) - 1;
    idtr.offset = (uint64_t)kmalloc(256 * sizeof(struct idtdescentry));
    memset((void*)idtr.offset, 0, 256 * sizeof(struct idtdescentry));

    //TODO: Maybe this has to be uncommented, i don't know if idt need to be user accessible!
    //struct page_directory* pml4 = get_pml4();
    //mprotect(pml4, (void*)idtr.offset, 256 * sizeof(struct idtdescentry), VMM_USER_BIT | VMM_WRITE_BIT);

    for (int i = 0; i < 256; i++) {
        set_idt_gate((uint64_t)interrupt_vector[i], i, IDT_TA_InterruptGate, 1, get_kernel_code_selector());
    }

    set_idt_gate((uint64_t)DoubleFault_Handler, 8, IDT_TA_InterruptGate, 1, get_kernel_code_selector());

    for (int i = 0; i < 32; i++) {
        dynamic_interrupt_handlers[i] = interrupt_exception_handler;
    }

    dynamic_interrupt_handlers[0x8] = DoubleFault_Handler;
    dynamic_interrupt_handlers[0xD] = GPFault_Handler;
    dynamic_interrupt_handlers[0xE] = PageFault_Handler;
    dynamic_interrupt_handlers[PCIA_IRQ] = PCI_Handler;
    dynamic_interrupt_handlers[PIT_IRQ] = PitInt_Handler;
    dynamic_interrupt_handlers[SR2_IRQ] = Serial2Int_Handler;
    dynamic_interrupt_handlers[SR1_IRQ] = Serial1Int_Handler;
    dynamic_interrupt_handlers[0x80] = Syscall_Handler;
    
    interrupts_ready = 1;
    return;
}

void raise_interrupt(uint8_t interrupt) {
    if (!interrupts_ready) panic("Interrupts not ready\n");
    dynamic_interrupt = interrupt;
    __asm__("int %0" : : "i"(DYNAMIC_HANDLER));
}

const char * get_io_tty() {
    return io_tty;
}

uint8_t global_interrupt_handler(cpu_context_t* ctx, uint8_t cpu_id) {
    //Get rflags
    uint64_t interrupt_number = ctx->interrupt_number;
    uint64_t rflags;
    __asm__ volatile("pushfq; pop %0" : "=r"(rflags));
    //If interrupts are enabled panic
    if (rflags & 0x200) {
        kprintf("Interrupts enabled in handler\n");
        panic("Interrupts enabled in handler\n");
    }

    if (!IS_EXCEPTION_INUM(interrupt_number)) notify_eoi_required(interrupt_number);

    //TODO: Get current proces
    thread_t * current_thread = get_current_thread();
    if (!current_thread) {
        panic("No current thread\n");
    }
    
    //kprintf("[PID: %d | TID %d] Interrupt %d on CPU %d\n", current_thread->process->pid, current_thread->id, interrupt_number, cpu_id);
    memcpy(current_thread->user_context->cpu_context, ctx, sizeof(cpu_context_t));
    memcpy(current_thread->user_context->cpu_context->info, ctx->info, sizeof(struct cpu_context_info));
    arch_simd_save_context(current_thread->user_context->fxsave_region);

    void (*handler)(cpu_context_t* ctx, uint8_t cpu_id) = (void*)dynamic_interrupt_handlers[interrupt_number];
    if (interrupt_number == DYNAMIC_HANDLER) {
        if (dynamic_interrupt != 0 && dynamic_interrupt != DYNAMIC_HANDLER) {   
            handler = (void*)dynamic_interrupt_handlers[dynamic_interrupt];
            dynamic_interrupt = 0;
        } else {
            panic("Invalid dynamic interrupt\n");
        }
    }

    //kprintf("Interrupt %d received on CPU %d\n", ctx->interrupt_number, cpu_id);

    if (handler == 0) {
        kprintf("No handler for interrupt ");
        kprintf("%d", interrupt_number);
        kprintf("\n");
        panic("No handler for interrupt !\n");
    }

    handler(ctx, cpu_id);

    current_thread = get_current_thread();
    if (!current_thread) {
        panic("No current thread\n");
    }
    //kprintf("[PID: %d | TID %d] Interrupt %d on CPU %d returning\n", current_thread->process->pid, current_thread->id, interrupt_number, cpu_id);
    arch_simd_restore_context(current_thread->user_context->fxsave_region);
    memcpy(ctx, current_thread->user_context->cpu_context, sizeof(cpu_context_t));
    memcpy(ctx->info, current_thread->user_context->cpu_context->info, sizeof(struct cpu_context_info));
    struct tss * tss = arch_get_cpu(cpu_id)->tss;
    tss_set_stack(tss, ctx->info->kstack, 0);
    tss_set_stack(tss, ctx->rsp, 3);
    setFsBase(current_thread->user_context->fs_base);

    if (IS_EXCEPTION_INUM(interrupt_number)) return 1;
    local_apic_eoi(cpu_id, interrupt_number);
    return 0;
}

uint8_t kswap_interrupt_handler(cpu_context_t* ctx, uint8_t cpu_id) {

    //TODO: Get current proces
    thread_t * current_thread = get_current_thread();
    thread_t * old_thread = current_thread;
    if (!current_thread) {
        panic("No current thread\n");
    }
    
    //kprintf("[PID: %d | TID %d] Interrupt %d on CPU %d\n", current_thread->process->pid, current_thread->id, interrupt_number, cpu_id);
    memcpy(current_thread->kernel_context->cpu_context, ctx, sizeof(cpu_context_t));
    memcpy(current_thread->kernel_context->cpu_context->info, ctx->info, sizeof(struct cpu_context_info));
    arch_simd_save_context(current_thread->kernel_context->fxsave_region);

    sched();

    current_thread = get_current_thread();
    if (!current_thread) {
        panic("No current thread\n");
    }
    //kprintf("[PID: %d | TID %d] Interrupt %d on CPU %d returning\n", current_thread->process->pid, current_thread->id, interrupt_number, cpu_id);
    if ((uint64_t)current_thread->kernel_context->cpu_context->rip == (uint64_t)emulate_syscall_return) {
        kprintf("KSWAP_IRQ handler returned with interrupt number %d\n", current_thread->kernel_context->cpu_context->interrupt_number);
        arch_simd_restore_context(old_thread->kernel_context->fxsave_region);
        memcpy(ctx, old_thread->kernel_context->cpu_context, sizeof(cpu_context_t));
        memcpy(ctx->info, old_thread->kernel_context->cpu_context->info, sizeof(struct cpu_context_info));
        ctx->rip = current_thread->kernel_context->cpu_context->rip;
        struct tss * tss = arch_get_cpu(cpu_id)->tss;
        tss_set_stack(tss, ctx->info->kstack, 0);
        tss_set_stack(tss, ctx->rsp, 3);
        setFsBase(old_thread->kernel_context->fs_base);
    } else {
        arch_simd_restore_context(current_thread->kernel_context->fxsave_region);
        memcpy(ctx, current_thread->kernel_context->cpu_context, sizeof(cpu_context_t));
        memcpy(ctx->info, current_thread->kernel_context->cpu_context->info, sizeof(struct cpu_context_info));
        struct tss * tss = arch_get_cpu(cpu_id)->tss;
        tss_set_stack(tss, ctx->info->kstack, 0);
        tss_set_stack(tss, ctx->rsp, 3);
        setFsBase(current_thread->kernel_context->fs_base);
    }

    return 0;
}

void int_hardcore_wrapper(cpu_context_t* ctx, uint8_t cpu_id) {

    if (eoi_pending()) {
        kprintf("Interrupt %d pending EOI\n", ctx->interrupt_number);
        if (ctx->interrupt_number != 0xe && ctx->interrupt_number != KSWAP_IRQ)
            panic("EOI pending entering interrupt handler\n");
    }

    uint8_t res = (ctx->interrupt_number == KSWAP_IRQ) ? kswap_interrupt_handler(ctx, cpu_id) : global_interrupt_handler(ctx, cpu_id);
    
    if (eoi_pending()) {
        if (res == 1) {
            kprintf("EOI pending returning from exception\n");
        } else {
            kprintf("EOI pending returning from interrupt\n");
        }
        if (ctx->interrupt_number != 0xe && ctx->interrupt_number != KSWAP_IRQ)
            panic("EOI pending returning from interrupt handler\n");
    }
}

void mask_interrupt(uint8_t irq) {
    if (!interrupts_ready) panic("Interrupts not ready\n");
    if (!ioapic_mask(irq, 0x0)) {
        panic("Failed to mask interrupt\n");
    }
}

void unmask_interrupt(uint8_t irq) {
    if (!interrupts_ready) panic("Interrupts not ready\n");
    kprintf("Unmasking interrupt %d\n", irq);
    if (!ioapic_mask(irq, 0x1)) {
        panic("Failed to unmask interrupt\n");
    }
}