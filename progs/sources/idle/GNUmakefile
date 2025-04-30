override PROGRAM := idle.elf

CC := /usr/bin/cc
LD := /usr/bin/ld
ASMC := nasm

PROGRAM_ENTRY := _start

CFLAGS ?= -O2 -g -Wall -Wextra -Wpedantic -pipe -std=c11
LDFLAGS ?=
NASMFLAGS ?= -F dwarf -g

ABSDIR := $(shell pwd)
LIBCDIR := $(ABSDIR)/../../minilibc
SRCDIR := src
BUILDHOME := build
BUILDDIR := build/bin
OBJDIR := build/lib
SYMDIR := build/sym

DIRS := $(wildcard $(SRCDIR)/*)

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Check that given variables are set and all have non-empty values,
# die with an error otherwise.
#
# Params:
#   1. Variable name(s) to test.
#   2. (optional) Error message to print.
check_defined = \
    $(strip $(foreach 1,$1, \
        $(call __check_defined,$1,$(strip $(value 2)))))
__check_defined = \
    $(if $(value $1),, \
      $(error Undefined $1$(if $2, ($2))))

override CFLAGS +=       \
    -I.                  \
    -I$(LIBCDIR)         \
    -std=c11             \
    -ffreestanding       \
    -fno-stack-protector \
    -fno-stack-check     \
    -fno-pie             \
    -fno-pic             \
    -m64                 \
    -march=x86-64        \
    -nostartfiles        \
    -nodefaultlibs       \
    -nostdlib            \
    -mabi=sysv           \
    -mno-80387           \
    -mno-red-zone        \
    -mcmodel=kernel      \
    -MMD
override CFLAGS += $(CEXTRA)

override LDFLAGS +=         \
    -nostdlib               \
    -static                 \
    -z max-page-size=0x1000 \
    -T linker.ld

override NASMFLAGS += \
    -f elf64

override LIBC_CFILES :=$(call rwildcard,$(LIBCDIR),*.c)
override LIBC_ASFILES := $(call rwildcard,$(LIBCDIR),*.S)
override LIBC_NASMFILES := $(call rwildcard,$(LIBCDIR),*.asm)

override OBJS := $(patsubst $(LIBCDIR)/%.c, $(OBJDIR)/%.o, $(LIBC_CFILES))
override OBJS += $(patsubst $(LIBCDIR)/%.S, $(OBJDIR)/%_s.o, $(LIBC_ASFILES))
override OBJS += $(patsubst $(LIBCDIR)/%.asm, $(OBJDIR)/%_asm.o, $(LIBC_NASMFILES))

override LIBC_HEADER_DEPS := $(LIBC_CFILES:.c=.d) $(LIBC_ASFILES:.S=.d) $(LIBC_NASMFILES:.asm=.d)

override SRC_CFILES :=$(call rwildcard,$(SRCDIR),*.c)        
override SRC_ASFILES := $(call rwildcard,$(SRCDIR),*.S)
override SRC_NASMFILES := $(call rwildcard,$(SRCDIR),*.asm)

override OBJS += $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRC_CFILES))
override OBJS += $(patsubst $(SRCDIR)/%.S, $(OBJDIR)/%_s.o, $(SRC_ASFILES))
override OBJS += $(patsubst $(SRCDIR)/%.asm, $(OBJDIR)/%_asm.o, $(SRC_NASMFILES))

override SRC_HEADER_DEPS := $(SRC_CFILES:.c=.d) $(SRC_ASFILES:.S=.d) $(SRC_NASMFILES:.asm=.d)

# You may comment @make run in this target if you intend
# to use virtualbox or real hardware.
all:
	@echo "Building program..."
	@echo "Absdir: $(ABSDIR)"
	@make program
	@echo "Generating symbol table..."
	@mkdir -p $(SYMDIR)
# Create symbol table for entering in gdb
	@objcopy --only-keep-debug $(BUILDDIR)/$(PROGRAM) $(SYMDIR)/$(PROGRAM).sym
	@echo "Done."

program: $(OBJS) link

$(OBJDIR)/%.o: $(SRCDIR)/%.c
#	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $^ -o $@

$(OBJDIR)/%_asm.o: $(SRCDIR)/%.asm
#	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	@$(ASMC) $^ -felf64 -o $@

$(OBJDIR)/%_s.o: $(SRCDIR)/%.S
#	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	@$(ASMC) $(NASMFLAGS) $^ -f elf64 -o $@

$(OBJDIR)/%.o: $(LIBCDIR)/%.c
#	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $^ -o $@

$(OBJDIR)/%_asm.o: $(LIBCDIR)/%.asm
#	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	@$(ASMC) $^ -felf64 -o $@

$(OBJDIR)/%_s.o: $(LIBCDIR)/%.S
#	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	@$(ASMC) $(NASMFLAGS) $^ -f elf64 -o $@

link: 
#	@ echo !==== LINKING $^
	@$(LD) $(LDFLAGS) -o $(BUILDDIR)/$(PROGRAM) $(OBJS)

clean:
	@rm -rf $(OBJDIR)/*
	@rm -rf $(OBJS)
	@rm -rf $(BUILDDIR)/*
	@rm -rf $(SYMDIR)