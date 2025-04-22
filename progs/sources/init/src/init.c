#include <minilibc.h>
#include <stdio.h>
#include <string.h>
#include <cpuid.h>
#include <auxv.h>

int main(int argc, char* argv[], char* envp[]) {
    printf("Init starting");
    printf("Argc: %d", argc);
    for (int i = 0; i < argc; i++) {
        printf("Argv[%d]: %s", i, argv[i]);
    }

    int envc = 0;
    if (envp != 0)
        for (envc = 0; envp[envc] != NULL; envc++);

    for (int i = 0; i < envc; i++) {
        printf("Envp[%d]: %s", i, envp[i]);
    }

    unsigned long long vsdo_address = getauxval(AT_SYSINFO_EHDR);
    printf("VDSO address: %p", vsdo_address);
}