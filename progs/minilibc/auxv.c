#include "auxv.h"
#include "string.h"

struct auxv *auxv;

char * get_auxv_string(unsigned long long type) {
    switch (type) {
        case AT_NULL:
            return "AT_NULL";
        case AT_IGNORE:
            return "AT_IGNORE";
        case AT_EXECFD:
            return "AT_EXECFD";
        case AT_PHDR:
            return "AT_PHDR";
        case AT_PHENT:
            return "AT_PHENT";
        case AT_PHNUM:
            return "AT_PHNUM";
        case AT_PAGESZ:
            return "AT_PAGESZ";
        case AT_BASE:
            return "AT_BASE";
        case AT_FLAGS:
            return "AT_FLAGS";
        case AT_ENTRY:
            return "AT_ENTRY";
        case AT_NOTELF:
            return "AT_NOTELF";
        case AT_UID:
            return "AT_UID";
        case AT_EUID:
            return "AT_EUID";
        case AT_GID:
            return "AT_GID";
        case AT_EGID:
            return "AT_EGID";
        case AT_PLATFORM:
            return "AT_PLATFORM";
        case AT_HWCAP:
            return "AT_HWCAP";
        case AT_CLKTCK:
            return "AT_CLKTCK";
        case AT_SECURE:
            return "AT_SECURE";
        case AT_BASE_PLATFORM:
            return "AT_BASE_PLATFORM";
        case AT_RANDOM:
            return "AT_RANDOM";
        case AT_HWCAP2:
            return "AT_HWCAP2";
        case AT_RSEQ_FEATURE_SIZE:
            return "AT_RSEQ_FEATURE_SIZE";
        case AT_RSEQ_ALIGN:
            return "AT_RSEQ_ALIGN";
        case AT_HWCAP3:
            return "AT_HWCAP3";
        case AT_HWCAP4:
            return "AT_HWCAP4";
        case AT_EXECFN:
            return "AT_EXECFN";
        case AT_SYSINFO_EHDR:
            return "AT_SYSINFO_EHDR";
        case AT_MINSIGSTKSZ:
            return "AT_MINSIGSTKSZ";
        default:
            return "Unknown auxv type";
    }
    return NULL;
}

void load_auxv(char ** envp) {
    if (envp == 0) return;

    int envc = 0;
    while (envp[envc] != NULL) envc++;
    envc++;

    auxv = (struct auxv*)&(envp[envc]);
}

unsigned long getauxval(unsigned long type) {
    if (auxv == NULL) {
        return 0;
    }
    int i = 0;
    while (auxv[i].a_type != AT_NULL) {
        if (auxv[i].a_type == type) {
            return (unsigned long)auxv[i].a_val;
        }
        i++;
    }
    return 0;
}

void dump_auxval() {
    if (auxv == NULL) {
        return;
    }
    int i = 0;
    while (auxv[i].a_type != AT_NULL) {
        printf("auxv[%d]: TYPE: %s VAL: %llx\n", i, get_auxv_string(auxv[i].a_type), auxv[i].a_val);
        i++;
    }
    printf("auxv[%d]: TYPE: %s VAL: %llx\n", i, get_auxv_string(auxv[i].a_type), auxv[i].a_val);
}