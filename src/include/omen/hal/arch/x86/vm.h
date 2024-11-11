#ifndef _X86_VM_H
#define _X86_VM_H

#include <generic/config.h>
#include <omen/libraries/std/stdint.h>

#define PAGE_WRITE_BIT     0x1
#define PAGE_USER_BIT      0x2
#define PAGE_NX_BIT        0x4
<<<<<<< HEAD
#define PAGE_CACHE_DISABLE_BIT 0x8
#define PAGE_COW_BIT       0x1
=======
#define PAGE_CACHE_DISABLE 0x8
>>>>>>> 1a314df58cc1f4265f30f0834be4a4dacbed469b

struct page_directory_entry {
    uint64_t present                   :1;
    uint64_t writeable                 :1;
    uint64_t user_access               :1;
    uint64_t write_through             :1;
    uint64_t cache_disabled            :1;
    uint64_t accessed                  :1;
    uint64_t ignored_3                 :1;
    uint64_t size                      :1; // 0 means page directory mapped
    uint64_t global                    :1;
<<<<<<< HEAD
    uint64_t cow                        :3;
=======
    uint64_t os                        :3;
>>>>>>> 1a314df58cc1f4265f30f0834be4a4dacbed469b
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
<<<<<<< HEAD
    uint64_t cow                 : 3;
=======
    uint64_t os                 : 3;
>>>>>>> 1a314df58cc1f4265f30f0834be4a4dacbed469b
    uint64_t page_ppn           : 28;
    uint64_t reserved_1         : 12; // must be 0
    uint64_t ignored_1          : 11;
    uint64_t execution_disabled : 1;
} __attribute__((__packed__));

struct page_directory {
    struct page_directory_entry entries[512];
} __attribute__((aligned(PAGE_SIZE)));

struct page_table {
    struct page_table_entry entries[512];
} __attribute__((aligned(PAGE_SIZE)));


struct page_map_index{
    uint64_t PDP_i;
    uint64_t PD_i;
    uint64_t PT_i;
    uint64_t P_i;
};
#endif