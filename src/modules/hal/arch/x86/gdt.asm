[bits 64]
GLOBAL _load_gdt

_load_gdt:  
    lgdt [rdi]
    mov ax, 0x10 ; ss kernel
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov rax, qword .finish
    push qword 0x8
    push rax
    o64 retfq
.finish:
    ret