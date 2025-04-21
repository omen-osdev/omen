#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/managers/mem/vmm.h>
#include <omen/managers/cpu/process.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/libraries/executables/loader.h>
#include <vfs/vfs.h>
#include <vfs/vfs_interface.h>

#include <elf.h> //TODO: Integrate our own elf.h

const char * elf_class[] = {
    "Invalid class",
    "ELF32",
    "ELF64"
};

const char * elf_data[] = {
    "Invalid data",
    "2's complement, little endian",
    "2's complement, big endian"
};

const char * elf_osabi[] = {
    "UNIX System V",
    "HP-UX",
    "NetBSD",
    "Linux",
    "GNU Hurd",
    "Solaris",
    "AIX",
    "IRIX",
    "FreeBSD",
    "Tru64",
    "Novell Modesto",
    "OpenBSD",
    "OpenVMS",
    "NonStop Kernel",
    "AROS",
    "Fenix OS",
    "CloudABI",
    "Stratus Technologies OpenVOS"
};

const char * elf_type[] = {
    "NONE",
    "REL",
    "EXEC",
    "DYN",
    "CORE"
};

const char * elf_machine[] = {
    "No machine",
    "AT&T WE 32100",
    "SPARC",
    "Intel 80386",
    "Motorola 68000",
    "Motorola 88000",
    "Reserved for future use (was EM_486)",
    "Intel 80860",
    "MIPS I Architecture",
    "IBM System/370 Processor",
    "MIPS RS3000 Little-endian",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Hewlett-Packard PA-RISC",
    "Reserved for future use",
    "Fujitsu VPP500",
    "Enhanced instruction set SPARC",
    "Intel 80960",
    "PowerPC",
    "64-bit PowerPC",
    "IBM System/390 Processor",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "NEC V800",
    "Fujitsu FR20",
    "TRW RH-32",
    "Motorola RCE",
    "Advanced RISC Machines ARM",
    "Digital Alpha",
    "Hitachi SH",
    "SPARC Version 9",
    "Siemens TriCore embedded processor",
    "Argonaut RISC Core, Argonaut Technologies Inc.",
    "Hitachi H8/300",
    "Hitachi H8/300H",
    "Hitachi H8S",
    "Hitachi H8/500",
    "Intel IA-64 processor architecture",
    "Stanford MIPS-X",
    "Motorola ColdFire",
    "Motorola M68HC12",
    "Fujitsu MMA Multimedia Accelerator",
    "Siemens PCP",
    "Sony nCPU embedded RISC processor",
    "Denso NDR1 microprocessor",
    "Motorola Star*Core processor",
    "Toyota ME16 processor",
    "STMicroelectronics ST100 processor",
    "Advanced Logic Corp. TinyJ embedded processor family",
    "AMD x86-64 architecture",
    "Sony DSP Processor",
    "Digital Equipment Corp. PDP-10",
    "Digital Equipment Corp. PDP-11",
    "Siemens FX66 microcontroller",
    "STMicroelectronics ST9+ 8/16 bit microcontroller",
    "STMicroelectronics ST7 8-bit microcontroller",
    "Motorola MC68HC16 Microcontroller",
    "Motorola MC68HC11 Microcontroller",
    "Motorola MC68HC08 Microcontroller",
    "Motorola MC68HC05 Microcontroller",
    "Silicon Graphics SVx",
    "STMicroelectronics ST19 8-bit microcontroller",
    "Digital VAX",
    "Axis Communications 32-bit embedded processor",
    "Infineon Technologies 32-bit embedded processor",
    "Element 14 64-bit DSP Processor",
    "LSI Logic 16-bit DSP Processor",
    "Donald Knuth's educational 64-bit processor",
    "Harvard University machine-independent object files",
    "SiTera Prism",
    "Atmel AVR 8-bit microcontroller",
    "Fujitsu FR30",
    "Mitsubishi D10V",
    "Mitsubishi D30V",
    "NEC v850",
    "Mitsubishi M32R",
    "Matsushita MN10300",
    "Matsushita MN10200",
    "picoJava",
    "OpenRISC 32-bit embedded processor",
    "ARC Cores Tangent-A5",
    "Tensilica Xtensa Architecture",
    "Alphamosaic VideoCore processor",
    "Thompson Multimedia General Purpose Processor",
    "National Semiconductor 32000 series",
    "Tenor Network TPC processor",
    "Trebia SNP 1000 processor",
    "STMicroelectronics (www.st.com) ST200 microcontroller"
};

const char * elf_version[] = {
    "Invalid version",
    "Current version"
};

#define DYNAMIC_LINKER_BASE_ADDRESS ((void*)0x40000000)

uint8_t parse_elf_file(uint8_t * buffer) {
    Elf64_Ehdr * header = (Elf64_Ehdr *) buffer;
    if (memcmp(header->e_ident, ELFMAG, SELFMAG) != 0) {
        kprintf("Invalid ELF magic\n");
        return 0;
    }

    kprintf("Valid elf header\n");
    return 1;
}

void readelf_32(uint8_t * buffer, uint64_t size) {
    kprintf("32 Bit elf header not supported\n");
}

void elf_readelf(uint8_t * buffer, uint64_t size) {
    if (!parse_elf_file(buffer)) return;

    Elf64_Ehdr * elf_header = (Elf64_Ehdr *) buffer;
    if (elf_header->e_ident[EI_CLASS] == ELFCLASS32) {
        readelf_32(buffer, size);
        return;
    }

    //readelf_64
    kprintf("ELF Header:\n");
    kprintf("  Magic: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", elf_header->e_ident[0], elf_header->e_ident[1], elf_header->e_ident[2], elf_header->e_ident[3], elf_header->e_ident[4], elf_header->e_ident[5], elf_header->e_ident[6], elf_header->e_ident[7], elf_header->e_ident[8], elf_header->e_ident[9], elf_header->e_ident[10], elf_header->e_ident[11], elf_header->e_ident[12], elf_header->e_ident[13], elf_header->e_ident[14], elf_header->e_ident[15]);
    kprintf("  Class: %s\n", elf_class[elf_header->e_ident[EI_CLASS]]);
    kprintf("  Data: %s\n", elf_data[elf_header->e_ident[EI_DATA]]);
    kprintf("  Version: %s\n", elf_version[elf_header->e_ident[EI_VERSION]]);
    kprintf("  OS/ABI: %s\n", elf_osabi[elf_header->e_ident[EI_OSABI]]);
    kprintf("  ABI Version: %d\n", elf_header->e_ident[EI_ABIVERSION]);
    kprintf("  Type: %s\n", elf_type[elf_header->e_type]);
    kprintf("  Machine: %s\n", elf_machine[elf_header->e_machine]);
    kprintf("  Version: 0x%x\n", elf_header->e_version);
    kprintf("  Entry point address: 0x%x\n", elf_header->e_entry);
    kprintf("  Start of program headers: %d (bytes into file)\n", elf_header->e_phoff);
    kprintf("  Start of section headers: %d (bytes into file)\n", elf_header->e_shoff);
    kprintf("  Flags: 0x%x\n", elf_header->e_flags);
    kprintf("  Size of this header: %d (bytes)\n", elf_header->e_ehsize);
    kprintf("  Size of program headers: %d (bytes)\n", elf_header->e_phentsize);
    kprintf("  Number of program headers: %d\n", elf_header->e_phnum);
    kprintf("  Size of section headers: %d (bytes)\n", elf_header->e_shentsize);
    kprintf("  Number of section headers: %d\n", elf_header->e_shnum);
    kprintf("  Section header string table index: %d\n", elf_header->e_shstrndx);
}

void allocate_segment(struct page_directory * root, Elf64_Phdr * program_header, void * buffer, void* base) {
    if (program_header->p_type != PT_LOAD) return;

    kprintf("Allocating on pml4: %llx, buffer: %llx, base: %llx\n", root, buffer, base);

    uint64_t vaddr_offset = program_header->p_vaddr & 0xfff;
    uint64_t vaddr = (program_header->p_vaddr & ~0xfff) + (uint64_t)base;
    if (vaddr >= 0x800000000000) {
        panic("Trying to map process into kernel space\n");
        return;
    }
    uint64_t total_size = program_header->p_memsz + vaddr_offset;
    uint64_t page_no = total_size / 0x1000;
    if (total_size % 0x1000) page_no++;

    uint8_t perms = VMM_USER_BIT;
    if (program_header->p_flags & PF_W) perms |= VMM_WRITE_BIT;
    if (!(program_header->p_flags & PF_X)) perms |= VMM_NX_BIT;

    void * new_buffer = kmalloc(page_no * 0x1000);
    memset(new_buffer, 0, page_no * 0x1000);
    memcpy(new_buffer + vaddr_offset,  (void*)(buffer+program_header->p_offset), program_header->p_filesz);
    map_range(root, (void*)vaddr, get_current_physical_address(new_buffer), 0x1000, page_no * 0x1000, perms);
}

uint8_t elf_open_file(char * filename, uint8_t ** buffer, uint64_t * filesize) {
    int fd = vfs_file_open(filename, 0, 0);
    if (fd < 0) {
        kprintf("Could not open file %s\n", filename);
        return 0;
    }

    vfs_file_seek(fd, 0, 0x2); //SEEK_END
    *filesize = (uint64_t)vfs_file_tell(fd);
    vfs_file_seek(fd, 0, 0x0); //SEEK_SET

    *buffer = kmalloc(*filesize);
    if (!*buffer) {
        kprintf("Could not allocate buffer for file\n");
        return 0;
    }

    memset(*buffer, 0, *filesize);
    vfs_file_read(fd, *buffer, *filesize);
    vfs_file_close(fd);
    
    return 1;
}


struct loaded_elf* elf_load_elf(struct page_directory* root, uint8_t * buffer, uint64_t size) {
    if (!parse_elf_file(buffer)) return NULL;

    Elf64_Ehdr * elf_header = (Elf64_Ehdr *) buffer;
    if (elf_header->e_ident[EI_CLASS] == ELFCLASS32) {
        readelf_32(buffer, size);
        return NULL;
    }

    //¿Es dinamico?
    if (elf_header->e_type == ET_DYN) {
        kprintf("Dynamic ELF not supported\n");
        return NULL;
    }

    if (elf_header->e_type != ET_EXEC) {
        kprintf("Invalid ELF type\n");
        return NULL;
    }

    Elf64_Phdr * program_header = (Elf64_Phdr *) (buffer + elf_header->e_phoff);
    struct proc_ld pld = {0};

    kprintf("Spawning process at 0x%x\n", elf_header->e_entry);

    for (int i = 0; i < elf_header->e_phnum; i++) {
        if (program_header[i].p_type == PT_LOAD) {
            allocate_segment(root, &program_header[i], buffer, 0);
        } else if (program_header[i].p_type == PT_PHDR) {
            pld.at_phdr = (void*)(program_header[i].p_vaddr);
        } else if (program_header[i].p_type == PT_INTERP) {
            if (pld.ld_path) {
                kprintf("Interp path already set\n");
                return NULL;
            }

            if (program_header[i].p_filesz > 0x1000) {
                kprintf("Interp path too long\n");
                return NULL;
            }

            pld.ld_path = kmalloc(program_header[i].p_filesz);
            memcpy(pld.ld_path, buffer + program_header[i].p_offset, program_header[i].p_filesz);
        }
    }
    
    struct auxv *vectors = kmalloc(sizeof(struct auxv) * 7);
    if (!vectors) {
        panic("Could not allocate auxv\n");
        return NULL;
    }
    memset(vectors, 0, sizeof(struct auxv) * 7);

    vectors[0].a_type = AT_ENTRY;
    vectors[0].a_val = (void*)elf_header->e_entry;
    vectors[1].a_type = AT_PHDR;
    vectors[1].a_val = (void*)pld.at_phdr;
    vectors[2].a_type = AT_PHENT;
    vectors[2].a_val = (void*)(uint64_t)elf_header->e_phentsize;
    vectors[3].a_type = AT_PHNUM;
    vectors[3].a_val = (void*)(uint64_t)elf_header->e_phnum;
    vectors[4].a_type = AT_BASE;
    vectors[4].a_val = (void*)(uint64_t)DYNAMIC_LINKER_BASE_ADDRESS;
    vectors[5].a_type = AT_PAGESZ;
    vectors[5].a_val = (void*)(uint64_t)0x1000;
    vectors[6].a_type = AT_SYSINFO_EHDR;
    vectors[6].a_val = (void*)(uint64_t)get_vdso_base();
    vectors[7].a_type = AT_NULL;
    vectors[7].a_val = 0;

    if (pld.ld_path) {
        kprintf("Dynamic linker path: %s\n", pld.ld_path);

        //Load dynamic linker
        uint8_t * ld_buffer;
        uint64_t ld_size;
        if (!elf_open_file(pld.ld_path, &ld_buffer, &ld_size)) {
            kprintf("Failed to open dynamic linker\n");
            return NULL;
        }

        if (!parse_elf_file(ld_buffer)) return 0;
        if (elf_header->e_ident[EI_CLASS] == ELFCLASS32) {
            readelf_32(ld_buffer, ld_size);
            return NULL;
        }

        Elf64_Ehdr * ld_elf_header = (Elf64_Ehdr *) ld_buffer;
        if (ld_elf_header->e_type != ET_DYN) {
            kprintf("Invalid ELF type\n");
            return NULL;
        }

        Elf64_Phdr * ld_program_header = (Elf64_Phdr *) (ld_buffer + ld_elf_header->e_phoff);
        for (int i = 0; i < ld_elf_header->e_phnum; i++) {
            if (ld_program_header[i].p_type == PT_LOAD) {
                allocate_segment(root, &ld_program_header[i], ld_buffer, DYNAMIC_LINKER_BASE_ADDRESS);
            }
        }

        kfree(ld_buffer);
        struct loaded_elf * ld = kmalloc(sizeof(struct loaded_elf));
        if (!ld) {
            panic("Could not allocate loaded elf\n");
            return NULL;
        }

        memset(ld, 0, sizeof(struct loaded_elf));
        ld->entry = (uint64_t)ld_elf_header->e_entry + (uint64_t)DYNAMIC_LINKER_BASE_ADDRESS;
        ld->auxv = vectors;
        ld->auxv_size = sizeof(struct auxv) * 7;
        ld->ld = &pld;
        ld->ld_size = sizeof(struct proc_ld);
        return ld;
    } else {

        struct loaded_elf * ld = kmalloc(sizeof(struct loaded_elf));
        if (!ld) {
            panic("Could not allocate loaded elf\n");
            return NULL;
        }

        memset(ld, 0, sizeof(struct loaded_elf));
        ld->entry = (uint64_t)elf_header->e_entry;
        ld->auxv = vectors;
        ld->auxv_size = sizeof(struct auxv) * 7;
        ld->ld = &pld;
        ld->ld_size = sizeof(struct proc_ld);
        return ld;
    }
}

char * get_auxv_string(uint64_t type) {
    switch (type) {
        case AT_NULL:
            return "AT_NULL";
        case AT_EXECFN:
            return "AT_EXECFN";
        case AT_PHDR:
            return "AT_PHDR";
        case AT_PHENT:
            return "AT_PHENT";
        case AT_PHNUM:
            return "AT_PHNUM";
        case AT_PAGESZ:
            return "AT_PAGESZ";
        case AT_BASE:
            return "AT_BASE";
        case AT_FLAGS:
            return "AT_FLAGS";
        case AT_ENTRY:
            return "AT_ENTRY";
        case AT_NOTELF:
            return "AT_NOTELF";
        case AT_SYSINFO_EHDR:
            return "AT_SYSINFO_EHDR";
        default:
            return "Unknown auxv type";
    }
    return NULL;
}