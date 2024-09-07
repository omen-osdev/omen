; Based on Kot's code: https://github.com/kot-org/Kot/blob/main/sources/core/kernel/source/arch/amd64/interrupts.s

extern global_interrupt_handler
global interrupt_vector

swapgs_if_necessary:
	cmp qword [rsp + 0x18], 0x8
	je donotswapgs
	swapgs
donotswapgs:
    ret

%macro interrupt_entry 1
    cmp	qword [rsp + 0x18], 0x8
    jz donot%1
    swapgs
    donot%1:
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
    
    mov rdi, rsp
    mov rsi, [gs:0x0]

    call global_interrupt_handler
    
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

    cmp	qword [rsp + 0x18], 0x8
    jz donot_%1
    swapgs
    donot_%1:

    add rsp, 16
    iretq
%endmacro

%macro INTERRUPT_WITHOUT_ERROR_CODE 1
interrupt_handler_%1:
    push 0
    push %1
    interrupt_entry %1
%endmacro

%macro INTERRUPT_WITH_ERROR_CODE 1
interrupt_handler_%1:
    push %1
    interrupt_entry %1
%endmacro

%macro CREATE_INTERRUPT_NAME 1  
    dq interrupt_handler_%1
%endmacro

INTERRUPT_WITHOUT_ERROR_CODE 0
INTERRUPT_WITHOUT_ERROR_CODE 1
INTERRUPT_WITHOUT_ERROR_CODE 2
INTERRUPT_WITHOUT_ERROR_CODE 3
INTERRUPT_WITHOUT_ERROR_CODE 4
INTERRUPT_WITHOUT_ERROR_CODE 5
INTERRUPT_WITHOUT_ERROR_CODE 6
INTERRUPT_WITHOUT_ERROR_CODE 7
INTERRUPT_WITH_ERROR_CODE   8
INTERRUPT_WITHOUT_ERROR_CODE 9
INTERRUPT_WITH_ERROR_CODE   10
INTERRUPT_WITH_ERROR_CODE   11
INTERRUPT_WITH_ERROR_CODE   12
INTERRUPT_WITH_ERROR_CODE   13
INTERRUPT_WITH_ERROR_CODE   14
INTERRUPT_WITHOUT_ERROR_CODE 15
INTERRUPT_WITHOUT_ERROR_CODE 16
INTERRUPT_WITH_ERROR_CODE   17
INTERRUPT_WITHOUT_ERROR_CODE 18
INTERRUPT_WITHOUT_ERROR_CODE 19
INTERRUPT_WITHOUT_ERROR_CODE 20
INTERRUPT_WITHOUT_ERROR_CODE 21
INTERRUPT_WITHOUT_ERROR_CODE 22
INTERRUPT_WITHOUT_ERROR_CODE 23
INTERRUPT_WITHOUT_ERROR_CODE 24
INTERRUPT_WITHOUT_ERROR_CODE 25
INTERRUPT_WITHOUT_ERROR_CODE 26
INTERRUPT_WITHOUT_ERROR_CODE 27
INTERRUPT_WITHOUT_ERROR_CODE 28
INTERRUPT_WITHOUT_ERROR_CODE 29
INTERRUPT_WITH_ERROR_CODE   30
INTERRUPT_WITHOUT_ERROR_CODE 31

%assign i 32
%rep 256 - i
    INTERRUPT_WITHOUT_ERROR_CODE i
%assign i i+1
%endrep

interrupt_vector:
    %assign i 0
    %rep 256
        CREATE_INTERRUPT_NAME i
    %assign i i+1
    %endrep