#ifndef _X86_VM_H
#define _X86_VM_H

#include <generic/config.h>
#include <omen/libraries/std/stdint.h>

struct page_directory_entry {
    uint64_t present                   :1;
    uint64_t writeable                 :1;
    uint64_t user_access               :1;
    uint64_t write_through             :1;
    uint64_t cache_disabled            :1;
    uint64_t accessed                  :1;
    uint64_t ignored_3                 :1;
    uint64_t size                      :1; // 0 means page directory mapped
    uint64_t ignored_2                 :4;
    uint64_t page_ppn                  :28;
    uint64_t reserved_1                :12; // must be 0
    uint64_t ignored_1                 :11;
    uint64_t execution_disabled        :1;
} __attribute__((packed));

struct page_table_entry{
    uint64_t present            : 1;
    uint64_t writeable          : 1;
    uint64_t user_access        : 1;
    uint64_t write_through      : 1;
    uint64_t cache_disabled     : 1;
    uint64_t accessed           : 1;
    uint64_t dirty              : 1;
    uint64_t size               : 1;
    uint64_t global             : 1;
    uint64_t ignored_2          : 3;
    uint64_t page_ppn           : 28;
    uint64_t reserved_1         : 12; // must be 0
    uint64_t ignored_1          : 11;
    uint64_t execution_disabled : 1;
} __attribute__((__packed__));

struct page_directory {
    struct page_directory_entry entries[512];
} __attribute__((aligned(PAGE_SIZE)));

typedef struct page_table {
    struct page_table_entry entries[512];
} __attribute__((aligned(PAGE_SIZE))) vm_t;
#endif