#include <omen/hal/arch/x86/int.h>
#include <omen/hal/arch/x86/io.h>
#include <omen/hal/arch/x86/apic.h>
#include <omen/hal/arch/x86/getcpuid.h>
#include <omen/hal/arch/x86/cpu.h>
#include <omen/hal/arch/x86/gdt.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/managers/mem/vmm.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/allocators/heap_allocator.h>

#define __UNDEFINED_HANDLER __asm__("cli"); kprintf(__func__); (void)frame; panic("Undefined interrupt handler");
#define IS_EXCEPTION(ctx)(ctx->interrupt_number < 32)

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

void (*dynamic_interrupt_handlers[256])(context_t* ctx, uint8_t cpuid) = {0};

void PageFault_Handler(context_t* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    uint64_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));
    kprintf("Page Fault Address: %lx\n", (uint64_t)faulting_address);
    kprintf("Error code: %lx\n", ctx->error_code);
    process_t * task = get_current_process();
    
    if (task) {
        if (remap_allocate_cow(task->vm, (void*)faulting_address)) {
            kprintf("COW'ed the shit out of %lx\n", (uint64_t)faulting_address);
            return;
        }
    } else {
        panic("On pagefault handler, can't get task\n");
    }

    panic("Page fault\n");
}

void DoubleFault_Handler(context_t* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    panic("Double fault\n");
}

void GPFault_Handler(context_t* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    panic("General protection fault\n");
}

void PCI_Handler(context_t* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    panic("PCI_Handler Not implemented\n");
}

void Syscall_Handler(context_t* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    panic("Syscall_Handler Not implemented\n");
}

//you may need save_all here
void PitInt_Handler(context_t* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    panic("PitInt_Handler Not implemented\n");
}

void Serial1Int_Handler(context_t* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    char c = inb(0x3f8);
    kprintf("Serial1: ");
    kprintf("%x", c);
    kprintf("\n");
}

void Serial2Int_Handler(context_t* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    char c = inb(0x3f8);
    kprintf("Serial2: ");
    kprintf("%x", c);
    kprintf("\n");
}

static void interrupt_exception_handler(context_t* ctx, uint8_t cpu_id) {
    printf("GENERIC EXCEPTION %d ON CPU %d\n", ctx->interrupt_number, cpu_id);
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
    mprotect_current((void*)idtr.offset, 256 * sizeof(struct idtdescentry), PAGE_USER_BIT | PAGE_WRITE_BIT);

    for (int i = 0; i < 256; i++) {
        set_idt_gate((uint64_t)interrupt_vector[i], i, IDT_TA_InterruptGate, 1, get_kernel_code_selector());
    }

    set_idt_gate((uint64_t)DoubleFault_Handler, 8, IDT_TA_InterruptGate, 1, get_kernel_code_selector());
    mprotect_current((void*)idtr.offset, 256 * sizeof(struct idtdescentry), PAGE_USER_BIT);

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

void global_interrupt_handler(context_t* ctx, uint8_t cpu_id) {
    if (!IS_EXCEPTION(ctx)) notify_eoi_required(ctx->interrupt_number);
    void (*handler)(context_t* ctx, uint8_t cpu_id) = (void*)dynamic_interrupt_handlers[ctx->interrupt_number];
    
    if (ctx->interrupt_number == DYNAMIC_HANDLER) {
        if (dynamic_interrupt != 0 && dynamic_interrupt != DYNAMIC_HANDLER) {   
            handler = (void*)dynamic_interrupt_handlers[dynamic_interrupt];
            dynamic_interrupt = 0;
        } else {
            panic("Invalid dynamic interrupt\n");
        }
    }

    //TODO: Get current proces

    //printf("Interrupt %d received on CPU %d\n", ctx->interrupt_number, cpu_id);

    if (handler == 0) {
        kprintf("No handler for interrupt ");
        kprintf("%d", ctx->interrupt_number);
        kprintf("\n");
        panic("No handler for interrupt !\n");
    }

    handler(ctx, cpu_id);

    if (IS_EXCEPTION(ctx)) return;
    
    if (ctx->interrupt_number == PIT_IRQ) {
        //if (requires_wakeup()) {
        //    wakeup();
        //    local_apic_eoi(cpu_id, ctx->interrupt_number);
        //} else if (requires_preemption()) {
        //    local_apic_eoi(cpu_id, ctx->interrupt_number);
        //    //YIELD
        //} else {
        //    local_apic_eoi(cpu_id, ctx->interrupt_number);
        //}
        //TODO: Context switch
        local_apic_eoi(cpu_id, ctx->interrupt_number);
    } else {
        local_apic_eoi(cpu_id, ctx->interrupt_number);
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
    printf("Unmasking interrupt %d\n", irq);
    if (!ioapic_mask(irq, 0x1)) {
        panic("Failed to unmask interrupt\n");
    }
}