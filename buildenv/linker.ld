OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)

PHDRS
{
    text    PT_LOAD FLAGS((1 << 0) | (1 << 2));
    rodata  PT_LOAD FLAGS((1 << 2));
    data    PT_LOAD FLAGS((1 << 1) | (1 << 2));
}

SECTIONS
{
    USER_START = 0x0000000000000000;
    USER_END = 0x00007fffffffffff;

    . = 0xffffffff80000000;
    KERNEL_START = .;

    .text : ALIGN(CONSTANT(MAXPAGESIZE)) {
        *(.text .text.*)
    } :text

    .data : ALIGN(CONSTANT(MAXPAGESIZE)){
        *(.data .data.*)
    } :data

    .bss : ALIGN(CONSTANT(MAXPAGESIZE)){
        *(COMMON)
        *(.bss .bss.*)
    } :data

    /DISCARD/ : {
        *(.note .note.*)
    }

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    KERNEL_END = .;
}