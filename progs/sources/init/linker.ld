OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(main)

PHDRS
{
    text    PT_LOAD FLAGS((1 << 0) | (1 << 2));
    rodata  PT_LOAD FLAGS((1 << 2));
    data    PT_LOAD FLAGS((1 << 1) | (1 << 2));
}

SECTIONS
{
    . = 0x100000;

    PROG_START = .;
    TEXT_START = .;
    .text : {
        *(.text .text.*)
    } : text

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    TEXT_END = .;
    DATA_START = .;
    .data : {
        *(.data .data.*)
    } : data

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    DATA_END = .;
    BSS_START = .;
    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    } : data

    /DISCARD/ : {
        *(.note .note.*)
    }

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    BSS_END = .;
    RODATA_START = .;
    .rodata : {
        *(.rodata .rodata.*)
    } : rodata

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    RODATA_END = .;

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    PROG_END = .;
}