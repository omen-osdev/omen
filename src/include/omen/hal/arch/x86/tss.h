#ifndef _X86_TSS_H
#define _X86_TSS_H

#include <omen/libraries/std/stdint.h>

#define TSS_RSP_SIZE 3
#define TSS_IST_SIZE 7

struct tss {
    uint32_t reserved0;
    uint64_t rsp[TSS_RSP_SIZE];
    uint64_t reserved1;
    uint64_t ist[TSS_IST_SIZE];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb;
} __attribute__((packed));

#endif