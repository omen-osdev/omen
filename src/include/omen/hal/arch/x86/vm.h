#ifndef _X86_VM_H
#define _X86_VM_H

#include <generic/config.h>
#include <omen/libraries/std/stdint.h>

#define PD_LEN 512
#define PT_LEN 512

#define VMM_WRITE_BIT           0x1
#define VMM_USER_BIT            0x2
#define VMM_NX_BIT              0x4
#define VMM_CACHE_DISABLE_BIT   0x8

#define VMM_CACHE_DISABLE_BIT_SET(x)(((x) & VMM_CACHE_DISABLE_BIT) >> 3)
#define VMM_NX_BIT_SET(x)(((x) & VMM_NX_BIT) >> 2)
#define VMM_USER_BIT_SET(x)(((x) & VMM_USER_BIT) >> 1)
#define VMM_WRITE_BIT_SET(x)((x) & VMM_WRITE_BIT)

typedef struct page_directory {
    uint64_t present : 1;
    uint64_t writeable : 1;
    uint64_t user_access : 1;
    uint64_t write_through : 1;
    uint64_t cache_disabled : 1;
    uint64_t accessed : 1;
    uint64_t ignored : 1;
    uint64_t size : 1;
    uint64_t ignored2 : 4;
    uint64_t page_ppn : 28;
    uint64_t reserved : 12;
    uint64_t ignored3 : 11;
    uint64_t execute_disable : 1;
} __attribute__((packed)) pd_entry;

typedef struct page_table {
    uint64_t present : 1;
    uint64_t writeable : 1;
    uint64_t user_access : 1;
    uint64_t write_through : 1;
    uint64_t cache_disabled : 1;
    uint64_t accessed : 1;
    uint64_t dirty : 1;
    uint64_t size : 1;
    uint64_t global : 1;
    uint64_t ignored : 3;
    uint64_t page_ppn : 28;
    uint64_t reserved : 12;
    uint64_t ignored2 : 11;
    uint64_t execute_disable : 1;
} __attribute__((packed)) pt_entry;

typedef struct pdTable {
    pd_entry entry[PD_LEN];
} __attribute__((packed)) pdTable;

typedef struct ptTable {
    pt_entry entry[PT_LEN];
} __attribute__((packed)) ptTable;

typedef struct pageMapIndex {
    uint64_t PDP_i;
    uint64_t PD_i;
    uint64_t PT_i;
    uint64_t P_i;
} pageMapIndex;
#endif