[BITS 64]

ALIGN 4096
extern global_syscall_handler
global syscall_entry
global syscall_enable

syscall_enable:
	; setup user gdt
	or		rsi, 0x3 ; or cs with ring 3 (userspace)
	sub		rsi, 0x10 ; or cs with ring
	; load segments into star msr
	mov		rcx, 0xc0000081
	rdmsr
	
	mov 	edx, edi
	sal 	esi, 16
	or      edx, esi
	wrmsr
	
	; load handler rip into lstar msr
	mov		rcx, 0xc0000082
	mov		rax, syscall_entry
	mov		rdx, rax
	shr		rdx, 32
	wrmsr

	; setup flags for syscall
	mov		rcx, 0xc0000084
	rdmsr
	or		eax, 0xfffffffe 
	wrmsr

	; enable syscall / sysret instruction
	mov		rcx, 0xc0000080
	rdmsr
	or		rax, 1 ; enable syscall extension
	wrmsr

	ret

syscall_entry:
    swapgs               ; swap from USER gs to KERNEL gs
    mov [gs:0x10], rsp    ; save current stack to the local cpu structure
    mov rsp, [gs:0x8]    ; use the kernel syscall stack
    mov rsp, [rsp]
    push qword rbp
    mov rbp, [gs:0x8]

    push qword [rbp + 0x10] ; ss
    push qword [gs:0x10]   ; rsp

    sti
    push r11             ; saved rflags
    push qword [rbp + 0x8] ; cs
    push rcx             ; current IP
    push 0x0             ; error code
    push 0x0             ; int number
    mov rbp, [rbp + 0x38] ; rbp

    push    r15
    push    r14
    push    r13
    push    r12
    push    r11
    push    r10
    push    r9
    push    r8
    push    rbp
    push    rdi
    push    rsi
    push    rdx
    push    rcx
    push    rbx
    push    rax
    push   qword [gs:0x8]
    mov     rax, cr3
    push    rax

    cld
    
    mov rax, [gs:0x8]
    mov rdi, rsp
    mov rsi, [rax + 0x18]

    call global_syscall_handler

    pop    rax
    mov    cr3, rax
    pop    qword [gs:0x8]
    pop    rax
    pop    rbx
    pop    rcx
    pop    rdx
    pop    rsi
    pop    rdi
    pop    rbp
    pop    r8
    pop    r9
    pop    r10
    pop    r11
    pop    r12
    pop    r13
    pop    r14
    pop    r15

    mov r11, [rsp + 0x20]
    mov rcx, [rsp + 0x10]

    cli
    mov rsp, [rsp + 0x28]
    swapgs
    o64 sysret