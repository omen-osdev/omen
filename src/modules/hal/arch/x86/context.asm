[bits 64]
global swap_context
global newctxcreat
global newsctxcreat
global newuctxcreat
global _signal_trampoline
extern returnoexit

; RSI stack pointer
; RDI init function
userspace_trampoline: 
    pop rsi ; Pop stack pointer
    pop rdi ; Pop init function

    push (4 * 8) | 3 ; CS
    push rsi ; Stack pointer
    push 0x200 ; RFLAGS 0010 0000 0000 
    ; this means 
    push (5 * 8) | 3 ; CS
    push rdi ; Init function

    mov rbp, rsi
    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    xor rsi, rsi
    xor rdi, rdi
    xor r8, r8
    xor r9, r9
    xor r10, r10
    xor r11, r11
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15

    iretq    

; RDI stack pointer
; RSI init function
newctxcreat:
    push rbx
    mov rbx, rsp
    mov rsp, QWORD [rdi]
    
    push returnoexit
    push 0x0
    push rsi

    mov QWORD [rdi], rsp
    mov rsp, rbx
    pop rbx
    ret

; RDI stack pointer
; RSI init function
newuctxcreat:
    push rbx
    mov rbx, rsp
    mov rsp, [rdi]

    push returnoexit
    push 0x0
    push rsi ; Init function
    push qword [rdi] ; Stack pointer
    push userspace_trampoline

    mov [rdi], rsp
    mov rsp, rbx
    pop rbx
    ret

; RDI stack pointer
; RSI init function
; RDX signal number
; RCX sigact
; r8 uctx
newsctxcreat:
    push rbx
    mov rbx, rsp
    mov rsp, [rdi]

    push rdx
    push rcx
    push r8
    push rsi
    push qword [rdi] ; Stack pointer

    mov [rdi], rsp
    mov rsp, rbx
    pop rbx
    ret