[bits 64]
global newctxswtch
global newctxcreat
global newuctxcreat
extern returnoexit

; Inputs:
; RDI: struct task* old
; RSI: struct task* new
; RDX: void* fxsave_area
; RCX: void* fxrstor_area


%define save(offset, register) mov [rdi + (8 * offset)], register
%define load(offset, register) mov register, [rsi + (8 * offset)]

; RDI old's context
; RSI new's context
; RDX old's fxsave_area
; RCX new's fxrstor_area
newctxswtch:
;first of all save rax
    save(2,rax)
    mov rax, cr3
    save(0,rax)
    save(3,rbx)
    save(4,rcx)
    save(5,rdx)
    save(6,rsi)
    save(7,rdi)
    save(8,rbp)
    save(9,r8)
    save(10,r9)
    save(11,r10)
    save(12,r11)
    save(13,r12)
    save(14,r13)
    save(15,r14)
    save(16,r15)
    xor rax, rax
    save(17,rax)
    save(18,rax)
    pop rax
    save(19,rax) ; Return address
    push rax
    mov rax, cs
    save(20,rax) ;cs
    pushfq
    pop rax
    or rax, 0x200 ; Interrupts will be enabled
    save(21,rax) ; rflags
    mov rax, rsp
    save(22,rax) ; rsp
    mov rax, ss
    save(23,rax) ; ss

    fxsave [rdx]

    ;Set IF bit in offset 21

    load(0,rax)
    mov cr3, rax

    fxrstor [rcx]

    load(3, rbx)
    load(4, rcx)
    load(5, rdx)
    load(8, rbp)
    load(9, r8)
    load(10, r9)
    load(11, r10)
    load(12, r11)
    load(13, r12)
    load(14, r13)
    load(15, r14)
    load(16, r15)
    load(19, rax) ; rip
    push rax 
    load(20, rax) ; cs
    load(21, rax) ; rflags
    push rax
    popfq
    load(22, rsp) ; rsp
    load(23, rax) ; ss
    load(2, rax)
    load(7, rdi)
    load(6, rsi)
    ret

; RSI stack pointer
; RDI init function
userspace_trampoline: 
    pop rsi ; Pop stack pointer
    mov rsi, [rsi]
    pop rdi ; Pop init function

    push (4 * 8) | 3 ; CS
    push rsi ; Stack pointer
    push 0x200 ; RFLAGS
    push (5 * 8) | 3 ; CS
    push rdi ; Init function
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
    push rdi ; Stack pointer
    push userspace_trampoline

    mov [rdi], rsp
    mov rsp, rbx
    pop rbx
    ret