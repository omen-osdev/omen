#ifndef _X86_VM_H
#define _X86_VM_H

#include <generic/config.h>
#include <omen/libraries/std/stdint.h>

#define PAGE_WRITE_BIT     0x1
#define PAGE_USER_BIT      0x2
#define PAGE_NX_BIT        0x4
#define PAGE_CACHE_DISABLE_BIT 0x8
#define PAGE_COW_BIT       0x1


#define MAXPHYADDR 40

//WARNING: WE ARE FIXING MAXPHYADDR = 40

struct directory_entry {
    uint64_t P                  :1; //0
    uint64_t RW                 :1; //1
    uint64_t US                 :1; //2
    uint64_t PWT                :1; //3
    uint64_t PCD                :1; //4
    uint64_t A                  :1; //5
    uint64_t IGNORED1           :1; //6 
    uint64_t PS                 :1; //7 reserved
    uint64_t IGNORED2           :3; //8-10
    uint64_t R                  :1; //11 ignored
    uint64_t PDPP               :28; //12-39
    uint64_t RESERVED           :12; //40-51
    uint64_t IGNORED3           :11; //52-62
    uint64_t XD                 :1; //63
} __attribute__((packed));

struct huge_entry {
    uint64_t P                  :1; //0
    uint64_t RW                 :1; //1
    uint64_t US                 :1; //2
    uint64_t PWT                :1; //3
    uint64_t PCD                :1; //4
    uint64_t A                  :1; //5

    uint64_t D                  :1; //6
    uint64_t PS                 :1; //7
    uint64_t G                  :1; //8
    uint64_t IGNORED1           :2; //9-10
    uint64_t R                  :1; //11
    uint64_t PAT                :1; //12
    uint64_t RESERVED           :17;//13-29
    uint64_t PDPP               :10;//30-39

    uint64_t RESERVED2          :12;//40-51
    uint64_t IGNORED2           :7; //52-58
    uint64_t PK                 :4; //59-62
    uint64_t XD                 :1; //63
} __attribute__((packed));

struct big_entry {
    uint64_t P                  :1; //0
    uint64_t RW                 :1; //1
    uint64_t US                 :1; //2
    uint64_t PWT                :1; //3
    uint64_t PCD                :1; //4
    uint64_t A                  :1; //5
    uint64_t D                  :1; //6
    uint64_t PS                 :1; //7
    uint64_t G                  :1; //8
    uint64_t IGNORED1           :2; //9-10
    uint64_t R                  :1; //11
    uint64_t PAT                :1; //12
    uint64_t RESERVED           :8; //13-20
    uint64_t PDPP               :19;//21-39
    uint64_t RESERVED2          :12;//40-51
    uint64_t IGNORED2           :7; //52-58
    uint64_t PK                 :4; //59-62
    uint64_t XD                 :1; //63
} __attribute__((packed));

struct regular_entry {
    uint64_t P                  :1; //0
    uint64_t RW                 :1; //1
    uint64_t US                 :1; //2
    uint64_t PWT                :1; //3
    uint64_t PCD                :1; //4
    uint64_t A                  :1; //5
    uint64_t D                  :1; //6
    uint64_t PAT                :1; //7
    uint64_t G                  :1; //8
    uint64_t IGNORED1           :2; //9-10
    uint64_t R                  :1; //11
    uint64_t PDPP               :28;//12-39
    uint64_t RESERVED           :12;//40-51
    uint64_t IGNORED2           :7; //52-58
    uint64_t PK                 :4; //59-62
    uint64_t XD                 :1; //63
} __attribute__((packed));

typedef union {
    struct directory_entry directory;
    struct huge_entry huge;
    struct big_entry big;
    struct regular_entry regular;
} __attribute__((packed)) vm_entry;
struct page_directory {
    uint64_t entries[512];
} __attribute__((aligned(PAGE_SIZE)));

struct page_map_index{
    uint64_t PML4_index;
    uint64_t PDP_index;
    uint64_t PD_index;
    uint64_t PT_index;
};
#endif