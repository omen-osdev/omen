section .text
global _start
global auxv
extern main
extern load_auxv
extern sys_exit

_start:                        ; _start is the entry point known to the linker
    xor rbp, rbp               ; effectively RBP := 0, mark the end of stack frames

    mov rdi, [rsp]             ; get argc from the stack (32-bit mov is OK; it zero-extends in 64-bit)
    lea rdi, [rsp + rdi*8 + 16]; prepare to send envp to the auxv initializer
    xor rax, rax
    call load_auxv

    mov rdi, [rsp]
    lea rsi, [rsp + 8]         ; take the address of argv from the stack
    lea rdx, [rsp + rdi*8 + 16]  ; take the address of envp from the stack
    xor rax, rax               ; per ABI and compatibility with icc
    call main                  ; edi, rsi, rdx are the three args to main

    mov rdi, rax               ; transfer the return of main to the first argument of _exit
    xor rax, rax               ; per ABI and compatibility with icc
    call sys_exit              ; terminate the program