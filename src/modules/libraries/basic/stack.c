#include <omen/libraries/basic/stack.h>
#include <omen/libraries/std/string.h>

//Stack grows downwards
void* push_u64(void * stack, uint64_t value) {
    stack -= 8;
    *(uint64_t*)stack = value;
    return stack;
}

void* push_str(void * stack, char * str) {
    int len = strlen(str);
    stack -= len;
    memcpy(stack, str, len);
    return stack;
}

uint64_t initialize_stack(void * stack, int argc, char* argv[], char *envp[], struct auxv *auxv) {
    int envc = 0;
    if(envp != 0){
        while(envp[envc] != NULL){
            envc++;
        }
    }

    //Stack layout
    //Auxv
    if (auxv != 0) {
        for (uint8_t i = 0; i < 5; i++) {
            if (&(auxv[i]) == (struct auxv*)0) continue;
            stack = push_u64(stack, auxv[i].a_type);
            stack = push_u64(stack, (uint64_t)auxv[i].a_val);
        }
    }
    //Envp
    for (int i = envc - 1; i >= 0; i--) {
        stack = push_str(stack, envp[i]);
    }

    //Allign stack adding nulls
    while ((uint64_t)stack % 16 != 0) {
        *(uint8_t*)stack = 0;
        stack++;
    }

    //Argv
    for (int i = argc - 1; i >= 0; i--) {
        stack = push_str(stack, argv[i]);
    }

    //Allign stack adding nulls
    while ((uint64_t)stack % 16 != 0) {
        *(uint8_t*)stack = 0;
        stack++;
    }
    
    //Argc
    stack = push_u64(stack, argc);

    return (uint64_t)stack;
}