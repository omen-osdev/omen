#ifndef _GDT_H
#define _GDT_H

#define GDT_MAX_CPU 32

//https://wiki.osdev.org/Global_Descriptor_Table
//https://github.com/kot-org/Kot/blob/main/Sources/Kernel/Src/arch/x86-64/gdt/gdt.h

#include <omen/libraries/std/stdint.h>

#define GDT_ENTRY_COUNT 6
#define GDT_SEGMENT (0b00010000)
#define GDT_PRESENT (0b10000000)
#define GDT_USER (0b01100000)
#define GDT_EXECUTABLE (0b00001000)
#define GDT_READWRITE (0b00000010)
#define GDT_LONG_MODE_GRANULARITY 0b0010
#define GDT_FLAGS 0b1100

#define GDT_NULL_ENTRY                 0x0
#define GDT_KERNEL_CODE_ENTRY          0x1
#define GDT_KERNEL_DATA_ENTRY          0x2
#define GDT_NULL_ENTRY_2               0x3
#define GDT_USER_DATA_ENTRY            0x4
#define GDT_USER_CODE_ENTRY            0x5
#define GDT_TSS_ENTRY                  0x6

#define GDT_DPL_KERNEL                  0x0
#define GDT_DPL_USER                    0x3

#define GDT_SYSTEM_TYPE_LDT             0x2
#define GDT_SYSTEM_TYPE_TSS_AVAILABLE   0x9
#define GDT_SYSTEM_TYPE_TSS_BUSY        0xB

#define GDT_EX_ENABLE                   0x1
#define GDT_EX_DISABLE                   0x0

#define MAX_GDT_ENTRIES ((65535) / sizeof(struct gdt_entry))

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t flags;
    uint8_t limit_middle : 4;
    uint8_t granularity : 4;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16_t length;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t flags1;
    uint8_t flags2;
    uint8_t base_high;
    uint32_t base_upper;
    uint32_t reserved;
} __attribute__((packed)) tss_entry_t;

struct gdt_descriptor {
    uint16_t size;
    uint64_t offset;
} __attribute__((packed));

typedef struct {
    gdt_entry_t entries[GDT_ENTRY_COUNT];
    tss_entry_t tss;
} __attribute__((packed)) gdt_t;

extern void _load_gdt(struct gdt_descriptor *gdt);

void create_gdt();
void load_gdt(uint8_t cpu);
struct tss *get_tss(uint64_t index);
uint16_t get_kernel_code_selector();
uint16_t get_kernel_data_selector();
uint16_t get_user_code_selector();
uint16_t get_user_data_selector();
void debug_gdt(uint8_t cpu);
#endif