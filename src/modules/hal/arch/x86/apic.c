//This code is based on Kot's implementation https://github.com/kot-org/Kot/blob/main/sources/core/kernel/source/arch/amd64/apic.c
#include <omen/hal/arch/x86/apic.h>
#include <omen/hal/arch/x86/capabilities.h>
#include <omen/hal/arch/x86/msr.h>
#include <omen/hal/arch/x86/io.h>
#include <omen/hal/arch/x86/int.h>
#include <omen/managers/mem/vmm.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/libraries/std/stdint.h>

struct apic_context actx;
volatile uint8_t apic_eoi_required = 0;
volatile uint64_t apic_last_interrupt = 0;

void write_lapic_register(void * lapic_addr, uint64_t offset, uint32_t value) {
    *((volatile uint32_t*)((void*)((uint64_t)lapic_addr+offset))) = value;
}

uint32_t read_lapic_register(void * lapic_addr, uint64_t offset) {
    return *((volatile uint32_t*)((void*)((uint64_t)lapic_addr+offset)));
}

uint32_t ioapic_read_register(void* apic_ptr, uint8_t offset) {
    *(volatile uint32_t*)(apic_ptr) = offset;
    return *(volatile uint32_t*)((uint64_t)apic_ptr+0x10);
}

void ioapic_write_register(void* apic_ptr, uint8_t offset, uint32_t value) {
    *(volatile uint32_t*)(apic_ptr) = offset;
    *(volatile uint32_t*)((uint64_t)apic_ptr+0x10) = value;
}

void debug_redirection_entry(struct ioapic_redirection_entry entry) {
    kprintf("Redirection Entry:\n");
    kprintf("Vector: %d\n", entry.vector);
    kprintf("Delivery Mode: %d\n", entry.delivery_mode);
    kprintf("Destination Mode: %d\n", entry.destination_mode);
    kprintf("Delivery Status: %d\n", entry.delivery_status);
    kprintf("Pin Polarity: %d\n", entry.pin_polarity);
    kprintf("Remote IRR: %d\n", entry.remote_irr);
    kprintf("Trigger Mode: %d\n", entry.trigger_mode);
    kprintf("Mask: %d\n", entry.mask);
    kprintf("Destination: %d\n", entry.destination);
}

void ioapic_set_redirection_entry(void* apic_ptr, uint64_t index, struct ioapic_redirection_entry entry) {
    volatile uint32_t low = (
        (entry.vector << IOAPIC_REDIRECTION_BITS_VECTOR) |
        (entry.delivery_mode << IOAPIC_REDIRECTION_BITS_DELIVERY_MODE) |
        (entry.destination_mode << IOAPIC_REDIRECTION_BITS_DESTINATION_MODE) |
        (entry.delivery_status << IOAPIC_REDIRECTION_BITS_DELIVERY_STATUS) |
        (entry.pin_polarity << IOAPIC_REDIRECTION_BITS_PIN_POLARITY) |
        (entry.remote_irr << IOAPIC_REDIRECTION_BITS_REMOTE_IRR) |
        (entry.trigger_mode << IOAPIC_REDIRECTION_BITS_TRIGGER_MODE) |
        (entry.mask << IOAPIC_REDIRECTION_BITS_MASK)
    );

    volatile uint32_t high = (
        (entry.destination << IOAPIC_REDIRECTION_BITS_DESTINATION)
    );

    //printf("Setting up redirection entry %d\n", index);
    //debug_redirection_entry(entry);

    ioapic_write_register(apic_ptr, IOAPIC_REDIRECTION_TABLE + (index * 2), low);
    ioapic_write_register(apic_ptr, IOAPIC_REDIRECTION_TABLE + (index * 2) + 1, high);
}

uint8_t account_apic_entry(uint64_t* entry) {
    struct entry_record* ce = (struct entry_record*)*entry;

    switch(ce->type) {
        case MADT_RECORD_TYPE_LAPIC:
            struct lapic* lapic = (struct lapic*)ce;
            if (lapic->apic_id > actx.max_apic_id) {
                actx.max_apic_id = lapic->apic_id;
            }
            actx.lapic_count++;
            break;
        case MADT_RECORD_TYPE_IOAPIC:
            actx.ioapic_count++;
            break;
        case MADT_RECORD_TYPE_IOAPIC_ISO:
            actx.ioapic_iso_count++;
            break;
        case MADT_RECORD_TYPE_IOAPIC_NMI:
            break;
        case MADT_RECORD_TYPE_LAPIC_NMI:
            break;
        case MADT_RECORD_TYPE_LAPIC_ADDR_OVERRIDE:
            break;
        case MADT_RECORD_TYPE_PLX2APIC:
            break;
        default:
            break;
    }

    uint8_t length = ce->length;
    *entry += length;
    return length;
}

uint8_t parse_apic_entry(uint64_t* entry) {
    struct entry_record* ce = (struct entry_record*)*entry;

    static uint8_t lapic_index = 0;
    static uint8_t ioapic_index = 0;
    static uint8_t ioapic_iso_index = 0;

    switch(ce->type) {
        case MADT_RECORD_TYPE_LAPIC:
            struct lapic* lapic = (struct lapic*)ce;
            actx.lapics[lapic_index++] = lapic;
            actx.lapic_address[lapic->apic_id] = (struct lapic_address*)kmalloc(sizeof(struct lapic_address));
            break;
        case MADT_RECORD_TYPE_IOAPIC:
            actx.ioapics[ioapic_index++] = (struct ioapic*)ce;
            break;
        case MADT_RECORD_TYPE_IOAPIC_ISO:
            actx.ioapic_isos[ioapic_iso_index++] = (struct ioapic_iso*)ce;
            break;
        case MADT_RECORD_TYPE_IOAPIC_NMI:
            break;
        case MADT_RECORD_TYPE_LAPIC_NMI:
            break;
        case MADT_RECORD_TYPE_LAPIC_ADDR_OVERRIDE:
            break;
        case MADT_RECORD_TYPE_PLX2APIC:
            break;
        default:
            break;
    }

    uint8_t length = ce->length;
    *entry += length;
    return length;
}

void* get_lapic_address() {
    uint8_t apic_id = getApicId();
    kprintf("APIC ID: %d\n", apic_id);
    return actx.lapic_address[apic_id]->virtual_address;
}

void enable_apic(uint8_t cpu_id) {
    uint32_t lo;
    uint32_t hi;
    cpuGetMSR(0x1B, &lo, &hi);

    uint64_t base_addr = (((uint64_t)hi << 32) | lo) & 0xFFFFF000;

    actx.lapic_address[cpu_id]->physical_address = (void*)base_addr;
    actx.lapic_address[cpu_id]->virtual_address = (void*)base_addr;

    void * lapic_address = get_lapic_address();

    uint64_t current_local_destination = read_lapic_register(lapic_address, LAPIC_LOGICAL_DESTINATION);
    uint64_t current_svr = read_lapic_register(lapic_address, LAPIC_SPURIOUS_INTERRUPT_VECTOR);

    write_lapic_register(lapic_address, LAPIC_DESTINATION_FORMAT, 0xffffffff);
    write_lapic_register(lapic_address, LAPIC_LOGICAL_DESTINATION, ((current_local_destination & ~((0xff << 24))) | (cpu_id << 24)));
    write_lapic_register(lapic_address, LAPIC_SPURIOUS_INTERRUPT_VECTOR, current_svr | (LOCAL_APIC_SPURIOUS_ALL | LOCAL_APIC_SPURIOUS_ENABLE_APIC));
    write_lapic_register(lapic_address, LAPIC_TASK_PRIORITY, 0);

    uint64_t value = ((uint64_t)actx.lapic_address[cpu_id]->physical_address) | (LOCAL_APIC_ENABLE & ~((1 << 10)));
    lo = value & 0xFFFFFFFF;
    hi = value >> 32;
    cpuSetMSR(0x1B, lo, hi);
}

void ioapic_init(uint64_t ioapic_id) {
    outb(PIC1_DATA, 0xff);
    outb(PIC2_DATA, 0xff);
    uint8_t apic_id = getApicId();
    kprintf("APIC ID: %d\n", apic_id);
    enable_apic(apic_id);

    struct ioapic* ioapic = actx.ioapics[ioapic_id];
    uint64_t ioapic_address = (uint64_t)ioapic->ioapic_address;
    map_current_memory((void*)ioapic_address, (void*)ioapic_address, PAGE_WRITE_BIT);
    uint64_t ioapic_version = ioapic_read_register(
        (void*)ioapic_address,
        IOAPIC_VERSION
    );

    uint8_t max_interrupts = ((ioapic_version >> 16) & 0xff) + 1;
    ioapic->max_interrupts = max_interrupts;

    kprintf("IOAPIC: %x\n", ioapic_address);
    kprintf("IOAPIC Version: %x\n", (uint8_t)ioapic_version);
    kprintf("IOAPIC Max Interrupts: %d\n", max_interrupts);

    uint32_t base = ioapic->gsi_base;
    
    for (uint64_t i = 0; i < max_interrupts; i++) {
        //printf("Setting up redirection entry %d\n", i);
        struct ioapic_redirection_entry entry = {
            .vector = IRQ_START + i,
            .delivery_mode = IOAPIC_REDIRECTION_DELIVERY_MODE_FIXED,
            .destination_mode = IOAPIC_REDIRECTION_DESTINATION_MODE_PHYSICAL,
            .delivery_status = IOAPIC_REDIRECTION_ENTRY_DELIVERY_STATUS_IDLE,
            .pin_polarity = IOAPIC_REDIRECTION_PIN_POLARITY_ACTIVE_HIGH,
            .remote_irr = IOAPIC_REDIRECTION_REMOTE_IRR_NONE,
            .trigger_mode = IOAPIC_REDIRECTION_TRIGGER_MODE_EDGE,
            .mask = IOAPIC_REDIRECTION_MASK_DISABLE,
            .destination = 0
        };

        ioapic_set_redirection_entry((void*)ioapic_address, i - base, entry);
    }

    for (uint64_t i = 0; i < actx.ioapic_iso_count; i++) {
        struct ioapic_iso* iso = actx.ioapic_isos[i];
        uint8_t irq_number = iso->irq_source + IRQ_START;
        struct ioapic_redirection_entry entry = {
            .vector = irq_number,
            .delivery_mode = IOAPIC_REDIRECTION_DELIVERY_MODE_FIXED,
            .destination_mode = IOAPIC_REDIRECTION_DESTINATION_MODE_PHYSICAL,
            .delivery_status = IOAPIC_REDIRECTION_ENTRY_DELIVERY_STATUS_IDLE,
            .pin_polarity = (iso->flags & 0x3) == 0x03 ? IOAPIC_REDIRECTION_PIN_POLARITY_ACTIVE_LOW : IOAPIC_REDIRECTION_PIN_POLARITY_ACTIVE_HIGH,
            .remote_irr = IOAPIC_REDIRECTION_REMOTE_IRR_NONE,
            .trigger_mode = (iso->flags & 0xc) == 0x0c ? IOAPIC_REDIRECTION_TRIGGER_MODE_LEVEL : IOAPIC_REDIRECTION_TRIGGER_MODE_EDGE,
            .mask = IOAPIC_REDIRECTION_MASK_DISABLE,
            .destination = 0
        };

        ioapic_set_redirection_entry((void*)ioapic_address, iso->irq_source, entry);
    }
}

void register_apic(struct madt_header * madt, char* (*cb)(void*, uint8_t, uint64_t)) {
    (void)cb;

    actx.lapic_count = 0;
    actx.ioapic_count = 0;
    actx.ioapic_iso_count = 0;
    actx.max_apic_id = 0;

    //printf("APIC: %x\n", madt->local_apic_address);
    //printf("flags: %x\n", madt->flags);
    //printf("length: %x\n", madt->header.length);

    uint64_t entry = (((uint64_t)madt)+sizeof(struct madt_header));
    uint64_t end = ((uint64_t)madt)+madt->header.length;
    while(entry < end) {
        uint8_t len = account_apic_entry(&entry);
        if(len == 0) {
            break;
        }
    }
    actx.lapics = (struct lapic**)kmalloc(sizeof(struct lapic)*actx.lapic_count);
    actx.lapic_address = (struct lapic_address**)kmalloc(sizeof(struct lapic_address*)* (actx.max_apic_id+1));
    actx.ioapics = (struct ioapic**)kmalloc(sizeof(struct ioapic)*actx.ioapic_count);
    actx.ioapic_isos = (struct ioapic_iso**)kmalloc(sizeof(struct ioapic_iso)*actx.ioapic_iso_count);

    entry = (((uint64_t)madt)+sizeof(struct madt_header));
    end = ((uint64_t)madt)+madt->header.length;
    while(entry < end) {
        uint8_t len = parse_apic_entry(&entry);
        if(len == 0) {
            break;
        }
    }

    for (uint64_t i = 0; i < actx.ioapic_count; i++) {
        ioapic_init(i);
    }

}

void local_apic_eoi(uint8_t cpu_id, uint64_t interrupt_number) {
    if (!apic_eoi_required) {
        panic("No EOI was requested\n");
    }
    if (interrupt_number != apic_last_interrupt) {
        panic("Invalid EOI request\n");
    }
    apic_eoi_required = 0;
    write_lapic_register(actx.lapic_address[cpu_id]->virtual_address, LAPIC_EOI, 0);
}

void notify_eoi_required(uint64_t interrupt_number){
    if (apic_eoi_required) {
        panic("EOI already requested\n");
    }
    apic_eoi_required = 1;
    apic_last_interrupt = interrupt_number;
}

uint64_t eoi_pending() {
    if (apic_eoi_required) {
        return apic_last_interrupt;
    } else {
        return 0;
    }
}

void io_change_irq_state(uint8_t irq, uint8_t io_apic_id, uint8_t is_enable){
    struct ioapic* ioapic = actx.ioapics[io_apic_id];
    if (ioapic == 0) {
        return;
    }
    uint64_t ioapic_address = (uint64_t)ioapic->ioapic_address;
    uint32_t base = ioapic->gsi_base;
    size_t index = irq - base;
    
    volatile uint32_t low = ioapic_read_register(
        (void*)ioapic_address,
        IOAPIC_REDIRECTION_TABLE + 2 * index
    );
    
    if(!is_enable){
        low |= 1 << IOAPIC_REDIRECTION_BITS_MASK;
    }else{
        low &= ~(1 << IOAPIC_REDIRECTION_BITS_MASK);
    }

    ioapic_write_register((void*)ioapic_address, IOAPIC_REDIRECTION_TABLE + 2 * index, low);
}

uint8_t ioapic_mask(uint8_t irq, uint8_t enable) {
    if(irq >= IRQ_START && (actx.ioapics[0]->max_interrupts + IRQ_START) > irq) {
        io_change_irq_state(irq - IRQ_START, 0, enable);
        return 1;
    }
    return 0;
}

uint8_t ioapic_get_max_interrupts() {
    return actx.ioapics[0]->max_interrupts;
}