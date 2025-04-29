[BITS 64]

ALIGN 4096
section .vdso
global _signal_trampoline

_signal_trampoline:
    pop rax ; Pop stack pointer
    pop rbx ; Pop init function
    pop rdx ; Pop uctx
    pop rsi ; Pop sigact
    pop rdi ; Pop signal number

    push rax ; Stack pointer
    push rbx ; Init function

    mov rbp, rax
    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor r8, r8
    xor r9, r9
    xor r10, r10
    xor r11, r11
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15

    ret