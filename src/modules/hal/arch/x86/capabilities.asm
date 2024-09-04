[bits 64]

%define fs_base             0xc0000100
%define gs_base             0xc0000101
%define gs_kernel_base      0xc0000102

;Get the cr0 register
getCr0:
    mov rax, cr0
    ret

;Get the cr2 register
getCr2:
    mov rax, cr2
    ret

;Get the cr3 register
getCr3:
    mov rax, cr3
    ret

;Get the cr4 register
getCr4:
    mov rax, cr4
    ret

;Get the cr8 register
getCr8:
    mov rax, cr8
    ret

getEfer:
    mov rax, 0xC0000080
    rdmsr
    ret

;Get the fs base msr
getFsBase:
    mov rax, 0xC0000100
    rdmsr
    ret

;Get the gs base msr
getGsBase:
    mov rax, 0xc0000101
    rdmsr
    ret

;Get the kernel gs base msr
getKernelGsBase:
    mov rax, 0xC0000102
    rdmsr
    ret

;Get the rflags register
getRflags:
    pushfq
    pop rax
    ret

;This code belongs to kot
need_cpuid:
    mov rax, 0x1
    cpuid
    shr rbx, 24
    mov rax, rbx
    ret

getApicId:
    mov ax, gs
    cmp ax, 0x0
    jnz need_cpuid
    mov rax, [gs:0x0]
    ret

reloadGsFs:
    mov ax, 0x0
    mov gs, ax
    mov fs, ax
    ret

setGsBase:
    mov eax, edi
    shr rdi, 32
    mov edx, edi
    mov ecx, gs_base
    wrmsr
    ret

setRflags:
    push rdi
    popfq
    ret

GLOBAL getCr0
GLOBAL getCr2
GLOBAL getCr3
GLOBAL getCr4
GLOBAL getCr8
GLOBAL getEfer
GLOBAL getFsBase
GLOBAL getGsBase
GLOBAL getKernelGsBase
GLOBAL getRflags
GLOBAL getApicId
GLOBAL reloadGsFs
GLOBAL setGsBase
GLOBAL setRflags