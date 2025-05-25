#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_STACK_SIZE 65536

struct auxv{
    unsigned long long a_type;
    void* a_val;
};

#define AT_NULL   0	/* end of vector */
#define AT_IGNORE 1	/* entry should be ignored */
#define AT_EXECFD 2	/* file descriptor of program */
#define AT_PHDR   3	/* program headers for program */
#define AT_PHENT  4	/* size of program header entry */
#define AT_PHNUM  5	/* number of program headers */
#define AT_PAGESZ 6	/* system page size */
#define AT_BASE   7	/* base address of interpreter */
#define AT_FLAGS  8	/* flags */
#define AT_ENTRY  9	/* entry point of program */
#define AT_NOTELF 10	/* program is not ELF */
#define AT_UID    11	/* real uid */
#define AT_EUID   12	/* effective uid */
#define AT_GID    13	/* real gid */
#define AT_EGID   14	/* effective gid */
#define AT_PLATFORM 15  /* string identifying CPU for optimizations */
#define AT_HWCAP  16    /* arch dependent hints at CPU capabilities */
#define AT_CLKTCK 17	/* frequency at which times() increments */
/* AT_* values 18 through 22 are reserved */
#define AT_SECURE 23   /* secure mode boolean */
#define AT_BASE_PLATFORM 24	/* string identifying real platform, may * differ from AT_PLATFORM. */
#define AT_RANDOM 25	/* address of 16 random bytes */
#define AT_HWCAP2 26	/* extension of AT_HWCAP */
#define AT_RSEQ_FEATURE_SIZE	27	/* rseq supported feature size */
#define AT_RSEQ_ALIGN		28	/* rseq allocation alignment */
#define AT_HWCAP3 29	/* extension of AT_HWCAP */
#define AT_HWCAP4 30	/* extension of AT_HWCAP */
#define AT_EXECFN  31	/* filename of program */
#define AT_SYSINFO_EHDR 33	/* sysinfo page */
#define AT_MINSIGSTKSZ	51	/* minimal stack size for signal delivery */

void dump_stack(void * stack, size_t size) {
    uint64_t * ptr = (uint64_t *)stack;
    printf("Dumping stack at address: %p, size: %zu bytes\n", stack, size);
    for (size_t i = 0; i < size / sizeof(uint64_t); i++) {
        printf("%p: %llx (%s)\n", (void *)(ptr + i), (unsigned long long)ptr[i], (char*)(ptr+i));
    }
    printf("End of stack dump\n");
}


void parse_stack(void * stack) {
    //Print the argc, argv, envp, auxv from the stack
    size_t * pointer_table = (size_t *)stack;
    size_t argc = *pointer_table++;
    printf("argc: %zu\n", argc);
    char ** argv = (char **)pointer_table;
    for (size_t i = 0; i < argc; i++) {
        if (argv[i] == NULL) {
            printf("argv[%zu] at %p points to NULL\n", i, (void *)&argv[i]);
        } else {
            printf("argv[%zu] at %p points to: %p, value: %s\n", i, (void *)&argv[i], (void *)argv[i], argv[i]);
        }
    }
    pointer_table += argc + 1; // Move past argv pointers
    char ** envp = (char **)pointer_table;
    size_t envp_count = 0;
    while (envp[envp_count] != NULL) {
        printf("envp[%zu] at %p points to: %p, value: %s\n", envp_count, (void *)&envp[envp_count], (void *)envp[envp_count], envp[envp_count]);
        envp_count++;
    }
    pointer_table += envp_count + 1; // Move past envp pointers
    struct auxv * auxv = (struct auxv *)pointer_table;
    size_t auxv_count = 0;
    while (auxv[auxv_count].a_type != AT_NULL) {
        printf("auxv[%zu]: type: %llu, value: %p\n", auxv_count, auxv[auxv_count].a_type, auxv[auxv_count].a_val);
        auxv_count++;
    }
    printf("End of auxv\n");
    printf("End of stack parsing\n");

}

void * create_vectors(void * stack, uint64_t max_size, char ** argv, char ** envp, struct auxv* auxv) {
    // Create the stack with the following layout:
    // HIGHEST ADDRESS (stack)
    // +------------------+
    // | envp strings     |
    // | argv strings     |
    // +------------------+ (stack - pointer_table_size)
    // | auxv entries     |
    // |------------------+
    // | envp pointers    |
    // | argv pointers    |
    // | argc             |
    // +------------------+ (stack - total_size)
    // LOWEST ADDRESS

    int argc = 0; while (argv[++argc] != NULL);
    int envc = 0; while (envp[++envc] != NULL);
    int auxc = 0; while (auxv[++auxc].a_type != AT_NULL);

    uint64_t argv_pointers[argc];
    uint64_t envp_pointers[envc];
#define PROTOSTACK_MAX_SIZE 4096
    uint64_t * ptr = malloc(PROTOSTACK_MAX_SIZE) - PROTOSTACK_MAX_SIZE;
    uint64_t original_addr = (uint64_t)ptr;
    printf("Argc at address: %p, value: %d\n", (void *)ptr, argc);
    *ptr++ = argc;
    for (int i = 0; i < argc; i++) {argv_pointers[i] = (uint64_t) ptr; *ptr = 0x1234; ptr++;}
    *ptr++ = 0x0;
    for (int i = 0; i < envc; i++) {envp_pointers[i] = (uint64_t) ptr; *ptr = 0x5678; ptr++;}
    *ptr++ = 0x0;
    for (int i = 0; i < auxc; i++) {*(struct auxv*)ptr = auxv[i]; ptr += 2;}
    ((struct auxv*)ptr)->a_type = AT_NULL;
    ((struct auxv*)ptr)->a_val  = NULL;
    ptr += 2;
    while ((uint64_t)ptr % 0xf) ptr++;
    
    char * ascii_ptr = (char *)ptr;
    for (int i = 0; i < argc; i++) {
        memcpy(ascii_ptr, argv[i], strlen(argv[i]) + 1);
        *(uint64_t*)argv_pointers[i] = (uint64_t)ascii_ptr;
        ascii_ptr += (strlen(argv[i]) + 1);
    }
    
    for (int i = 0; i < envc; i++) {
        memcpy(ascii_ptr, envp[i], strlen(envp[i]) + 1);
        *(uint64_t*)envp_pointers[i] = (uint64_t)ascii_ptr;
        ascii_ptr += (strlen(envp[i]) + 1);
    }

    uint64_t size = (uint64_t)ascii_ptr - original_addr;
    //Copy ptr to the stack at the end of the stack
    if (size > max_size) {
        fprintf(stderr, "Stack size exceeds maximum allowed size\n");
        free((void *)(original_addr - PROTOSTACK_MAX_SIZE));
        return NULL;
    }
    if (stack == NULL) {
        fprintf(stderr, "Stack pointer is NULL\n");
        free((void *)(original_addr - PROTOSTACK_MAX_SIZE));
        return NULL;
    }

    // Copy the stack to the provided stack pointer

    memcpy(stack - size, (void *)(original_addr), size);
    dump_stack(stack - size, size);
    free((void *)(original_addr + PROTOSTACK_MAX_SIZE));
    return stack - size;
}

void * add_vector(const char * str) {
    if (str == NULL) return NULL;
    size_t len = strlen(str) + 1; // +1 for null terminator
    void * new_vector = malloc(len);
    if (new_vector == NULL) {
        fprintf(stderr, "Memory allocation failed for vector\n");
        return NULL;
    }
    memcpy(new_vector, str, len);
    return new_vector;
}

int main(int cargc, char** cargv) {
    void * stack = malloc(MAX_STACK_SIZE);
    if (stack == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    memset(stack, 0, MAX_STACK_SIZE);

    char **argv = malloc(sizeof(char*) * (2)); // Program name and NULL terminator
    if (argv == NULL) {
        fprintf(stderr, "Memory allocation for argv failed\n");
        free(stack);
        return 1;
    }

    argv[0] = add_vector("/usr/bin/bash"); // Placeholder for program name
    argv[1] = add_vector(NULL); // NULL terminator for argv

    char **envp = malloc(sizeof(char*) * 9); // Environment variables, NULL terminated
    if (envp == NULL) {
        fprintf(stderr, "Memory allocation for envp failed\n");
        free(argv);
        free(stack);
        return 1;
    }

    envp[0] = add_vector("HOME=/usr");
    envp[1] = add_vector("PWD=/usr");
    envp[2] = add_vector("PATH=/usr/bin");
    envp[3] = add_vector("PATHSTORE=/usr/store");
    envp[4] = add_vector("USER=root");
    envp[5] = add_vector("HOSTNAME=omen");
    envp[6] = add_vector("TERM=gnome-256color");
    envp[7] = add_vector("PS1=\\[\033[0;92m$USER@$HOSTNAME\033[0;37m: \033[0;94m\\w\033[0;37m\\$\033[0m\\] ");
    envp[8] = add_vector(NULL); // NULL terminator for envp

    struct auxv * auxv = malloc(sizeof(struct auxv) * 8); // Auxiliary vector entries, NULL terminated
    if (auxv == NULL) {
        fprintf(stderr, "Memory allocation for auxv failed\n");
        free(envp);
        free(argv);
        free(stack);
        return 1;
    }

    auxv[0].a_type = AT_ENTRY;
    auxv[0].a_val = (void*)0x4001230; // Example entry point address
    auxv[1].a_type = AT_PHDR;
    auxv[1].a_val = (void*)0x4001231; // Example program header address
    auxv[2].a_type = AT_PHENT;
    auxv[2].a_val = (void*)0x4001232; // Example size of program header entry
    auxv[3].a_type = AT_PHNUM;
    auxv[3].a_val = (void*)0x4001233; // Example number of program headers
    auxv[4].a_type = AT_BASE;
    auxv[4].a_val = (void*)0x4001234; // Example base address of interpreter
    auxv[5].a_type = AT_PAGESZ;
    auxv[5].a_val = (void*)0x4001235; // Example system page size
    auxv[6].a_type = AT_SYSINFO_EHDR;
    auxv[6].a_val = (void*)0x4001236; // Example sysinfo page address
    auxv[7].a_type = AT_NULL; // End of vector
    auxv[7].a_val = NULL; // NULL terminator for auxv
    
    stack = create_vectors(stack, MAX_STACK_SIZE, argv, envp, auxv);
    if (stack == NULL) {
        fprintf(stderr, "Failed to create vectors\n");
        free(auxv);
        free(envp);
        free(argv);
        free(stack);
        return 1;
    }

    //Print the stack

    printf("Stack created successfully at address: %p\n", stack);
    parse_stack(stack);
    return 0;
}