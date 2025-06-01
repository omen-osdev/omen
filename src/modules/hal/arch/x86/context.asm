[bits 64]
global newctxcreat
global newsctxcreat
global newuctxcreat
global save_context
global load_context
extern returnoexit

save_context:
    mov QWORD [rdi + 0x10], rax ; rax
    mov rax, cr3
    mov QWORD [rdi + 0x00], rax
    mov rax, qword [gs:0x8]
    mov QWORD [rdi + 0x08], rax ; info
    mov QWORD [rdi + 0x18], rbx
    mov QWORD [rdi + 0x20], rcx
    mov QWORD [rdi + 0x28], rdx
    mov QWORD [rdi + 0x30], rsi
    mov QWORD [rdi + 0x38], rdi
    mov QWORD [rdi + 0x40], rbp
    mov QWORD [rdi + 0x48], r8
    mov QWORD [rdi + 0x50], r9
    mov QWORD [rdi + 0x58], r10
    mov QWORD [rdi + 0x60], r11
    mov QWORD [rdi + 0x68], r12
    mov QWORD [rdi + 0x70], r13
    mov QWORD [rdi + 0x78], r14
    mov QWORD [rdi + 0x80], r15

    pushf
    pop rax
    mov QWORD [rdi + 0xa8], rax ; rflags

    mov QWORD [rdi + 0xb0], rsp

    mov QWORD [rdi + 0x88], 0x0 ; interrupt number
    mov QWORD [rdi + 0x90], 0x0 ; error code

    ;save rip tro rdi + 0x98
    mov QWORD [rdi + 0x98], 0x0 ; rip
    mov QWORD [rdi + 0xa0], 0x0; cs
    mov QWORD [rdi + 0xb8], 0x0 ; ss
    ret

load_context:
    mov rax, QWORD [rdi + 0x00] ; cr3
    mov cr3, rax
    mov rax, QWORD [rdi + 0x08] ; info
    mov qword [gs:0x8], rax

    mov rax, QWORD [rdi + 0x10] ; rax
    mov rbx, QWORD [rdi + 0x18] ; rbx
    mov rcx, QWORD [rdi + 0x20] ; rcx
    mov rdx, QWORD [rdi + 0x28] ; rdx
    mov rsi, QWORD [rdi + 0x30] ; rsi
    mov rbp, QWORD [rdi + 0x40] ; rbp
    mov r8, QWORD [rdi + 0x48]   ; r8
    mov r9, QWORD [rdi + 0x50]   ; r9
    mov r10, QWORD [rdi + 0x58]  ; r10
    mov r11, QWORD [rdi + 0x60]  ; r11
    mov r12, QWORD [rdi + 0x68]  ; r12
    mov r13, QWORD [rdi + 0x70]  ; r13
    mov r14, QWORD [rdi + 0x78]  ; r14
    mov r15, QWORD [rdi + 0x80]  ; r15

    mov rax, QWORD [rdi + 0xa8] ; rflags
    push rax
    popf ; Restore RFLAGS

    ; Load the stack pointer from the context structure.
    mov rsp, QWORD [rdi + 0xb0]

    mov rax, QWORD [rdi + 0x10]
    mov rdi, QWORD [rdi + 0x38] ; rdi

    ret



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