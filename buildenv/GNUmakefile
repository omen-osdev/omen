override KERNEL := kernel.elf
override ISO := limine-cd.iso
override IMG := limine-cd.img
override VDI := limine-cd.vdi
override IMG_RAW := raw.img
override LIMINECFG := limine.conf
override GDBCFG := debug.gdb
override VMNAME := omen
override WDIR := omendata
override LOCKFILE := lockfile

# It is highly recommended to use a custom built cross toolchain to build a kernel.
# We are only using "cc" as a placeholder here. It may work by using
# the host system's toolchain, but this is not guaranteed.
 
CC := gcc
LD := ld
ASMC := nasm

# This are specific to my setup, please modify them!!!!
#########################DESKTOP SETTINGS#################################
#QEMU := "/mnt/c/Program Files/qemu/qemu-system-x86_64.exe"
#GDB := "/mnt/c/Users/85562/crossgdb/gdb-12.1/gdb/gdb"
#WSLHOSTIP := $(shell ipconfig.exe | grep 'WSL' -a -A4 | tail -n1 | cut -d":" -f 2 | tail -n1 | sed -e 's/\s*//g')
#########################LAPTOP SETTINGS###################################
QEMU := qemu-system-x86_64
GDB := gdb
WSLHOSTIP := 127.0.0.1
###########################################################################
MNTDIR := /mnt/omen

KERNEL_ENTRY := _start

BLOCKSIZE := 1024
MEMSIZE := 4G
VMEMSIZE := 1024M
NUMAFLAGS := \
-smp cpus=16 \
-object memory-backend-ram,size=1G,id=mem0 \
-object memory-backend-ram,size=1G,id=mem1 \
-object memory-backend-ram,size=1G,id=mem2 \
-object memory-backend-ram,size=1G,id=mem3 \
-numa node,memdev=mem0,cpus=0-3,nodeid=0 \
-numa node,memdev=mem1,cpus=4-7,nodeid=1 \
-numa node,memdev=mem2,cpus=8-11,nodeid=2 \
-numa node,memdev=mem3,cpus=12-15,nodeid=3 \
-numa dist,src=0,dst=1,val=15 \
-numa dist,src=2,dst=3,val=15 \
-numa dist,src=0,dst=2,val=20 \
-numa dist,src=0,dst=3,val=20 \
-numa dist,src=1,dst=2,val=20 \
-numa dist,src=1,dst=3,val=20

QFLAGS ?= -cpu qemu64 -d int -no-reboot -no-shutdown -machine q35 $(NUMAFLAGS) -m $(MEMSIZE) -boot d -serial stdio -serial telnet::4444,server,nowait -cdrom
QFLAGSEXP ?= -cpu qemu64 -d int -no-reboot -no-shutdown -machine q35 $(NUMAFLAGS) -m $(MEMSIZE) -boot d -serial stdio -drive if=pflash,format=raw,unit=0,file=$(OVMFDIR)/OVMF_CODE-pure-efi.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=$(OVMFDIR)/OVMF_VARS-pure-efi.fd -drive file=
#QWFLAGSEXP ?= -cpu qemu64 -d cpu_reset -machine q35 $(NUMAFLAGS) -m $(MEMSIZE) -boot d -netdev tap,id=mynet0,ifname=tap,script=no,downscript=no -serial stdio -device e1000,netdev=mynet0,mac=51:52:53:54:55:56 -drive if=pflash,format=raw,unit=0,file=$(WINDIRECTORY)/OVMF_CODE-pure-efi.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=$(WINDIRECTORY)/OVMF_VARS-pure-efi.fd -drive file=

CFLAGS ?= -O0 -g -Wall -Werror -Wextra -pipe -Wno-packed-bitfield-compat -std=c11
NASMFLAGS ?= -F dwarf -g
LDFLAGS ?=
CPPFLAGS ?=
GDBFLAGS ?=  --nx --command=$(ABSDIR)/$(GDBCFG)
GDBPORT ?= 1234

ABSDIR := $(shell cd ..; pwd)
LMNDIR := $(shell pwd)/limine
OVMFDIR := $(shell pwd)/OVMFbin
SRCDIR := $(ABSDIR)/src
INCDIR := $(SRCDIR)/include
CONFDIR := $(SRCDIR)/conf
DRVDIR := $(SRCDIR)/drivers
APPDIR := $(SRCDIR)/apps
BUILDHOME := $(ABSDIR)/build
BUILDDIR := $(ABSDIR)/build/bin
OBJDIR := $(ABSDIR)/build/lib
ISOBUILDDIR := $(ABSDIR)/build/iso_root
ISODIR := $(ABSDIR)/build/image
PROGSDIR := $(ABSDIR)/progs/sources
PROGSBUILDDIR := $(ABSDIR)/progs/export
PROGSDATADIR := $(ABSDIR)/progs/data
PROGSSYMDIR := $(ABSDIR)/progs/sym
PROGS := $(wildcard $(PROGSDIR)/*)
SYMDIR := /mnt/c/omen/

# /mnt/omen
SYSDIR := $(MNTDIR)/system
DATDIR := $(MNTDIR)/data

LMN := $(LMNDIR)/limine-deploy.exe
BOOTEFI := $(LMNDIR)/BOOTX64.EFI

LMNREPO := https://github.com/limine-bootloader/limine.git
LMNBRCH := v8.x-binary

DIRS := $(wildcard $(SRCDIR)/*)
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))
check_defined = \
    $(strip $(foreach 1,$1, \
        $(call __check_defined,$1,$(strip $(value 2)))))
__check_defined = \
    $(if $(value $1),, \
      $(error Undefined $1$(if $2, ($2))))

override CFLAGS +=       \
    -I.                  \
	-I$(INCDIR)          \
	-I$(CONFDIR)         \
	-I$(DRVDIR)          \
	-I$(APPDIR)          \
    -std=c11             \
    -ffreestanding       \
    -fno-pie             \
    -fno-pic             \
    -m64                 \
    -Wno-int-conversion  \
    -fno-stack-protector \
    -march=x86-64        \
    -mabi=sysv           \
    -mno-80387           \
    -mno-red-zone        \
    -mcmodel=kernel      \
    -MMD

override LDFLAGS +=         \
    -nostdlib               \
    -static                 \
    -z max-page-size=0x1000 \
    -T linker.ld

override NASMFLAGS += \
    -f elf64

override CFILES :=$(call rwildcard,$(SRCDIR),*.c)        
override ASFILES := $(call rwildcard,$(SRCDIR),*.S)
override NASMFILES := $(call rwildcard,$(SRCDIR),*.asm)
override OBJS := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(CFILES))
override OBJS += $(patsubst $(SRCDIR)/%.S, $(OBJDIR)/%_s.o, $(ASFILES))
override OBJS += $(patsubst $(SRCDIR)/%.asm, $(OBJDIR)/%_asm.o, $(NASMFILES))
override HEADER_DEPS := $(CFILES:.c=.d) $(ASFILES:.S=.d)

deps:
	@apt update
	@apt install git make build-essential nasm parted dosfstools mtools tree qemu-system gdb

# You may comment @make run in this target if you intend
# to use virtualbox or real hardware.
all:
	@echo "Building kernel..."
	@make kernel
	@echo "Building ISO..."
	@make buildimg
	@echo "Running QEMU..."
	@make run
	@echo "Done!"

kernel: $(OBJS) link

$(OBJDIR)/apps/%.o: $(SRCDIR)/apps/%.c
#	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	@$(CC) $(CFLAGS) -mgeneral-regs-only -c $^ -o $@

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

link: 
#	@ echo !==== LINKING $^
	@$(LD) $(LDFLAGS) -o $(BUILDDIR)/kernel.elf $(OBJS)

clean:
	@rm -rf $(OBJDIR)/*
	@rm -rf $(OBJS)
	@rm -f $(ISOBUILDDIR)/$(KERNEL)
	@rm -f $(ISOBUILDDIR)/$(LIMINECFG)
	@rm -f $(BUILDDIR)/$(KERNEL)
	@rm -f $(ISODIR)/$(IMG)
	@rm -f $(ISODIR)/$(ISO)
	@rm -f $(ISODIR)/$(VDI)

setup:
	@rm -rf $(LMNDIR)
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(ISOBUILDDIR)
	@mkdir -p $(PROGSBUILDDIR)
	@mkdir -p $(PROGSSYMDIR)
	@mkdir -p $(ISODIR)
	@dd if=/dev/zero of=$(ISODIR)/$(IMG_RAW) bs=4096 count=102400
	@git clone $(LMNREPO) --branch=$(LMNBRCH) --depth=1
	@cp -v $(LMNDIR)/limine-bios.sys $(LMNDIR)/limine-bios-cd.bin $(LMNDIR)/limine-uefi-cd.bin $(ISOBUILDDIR)
	@echo file $(BUILDDIR)/$(KERNEL) > $(ABSDIR)/$(GDBCFG)
	@echo target remote $(WSLHOSTIP):$(GDBPORT) >> $(ABSDIR)/$(GDBCFG)
	@echo set disassembly-flavor intel >> $(ABSDIR)/$(GDBCFG)
	@echo define adds >> $(ABSDIR)/$(GDBCFG)
	@echo if $$\argc == 1 >> $(ABSDIR)/$(GDBCFG)
	@echo set confirm off >> $(ABSDIR)/$(GDBCFG)
	@echo add-symbol-file $(PROGSSYMDIR)/$$\arg0.elf.sym >> $(ABSDIR)/$(GDBCFG)
	@echo set confirm on >> $(ABSDIR)/$(GDBCFG)
	@echo end >> $(ABSDIR)/$(GDBCFG)
	@echo end >> $(ABSDIR)/$(GDBCFG)
	@echo define rms >> $(ABSDIR)/$(GDBCFG)
	@echo if $$\argc == 1 >> $(ABSDIR)/$(GDBCFG)
	@echo set confirm off >> $(ABSDIR)/$(GDBCFG)
	@echo remove-symbol-file $(PROGSSYMDIR)/$$\arg0.elf.sym >> $(ABSDIR)/$(GDBCFG)
	@echo set confirm on >> $(ABSDIR)/$(GDBCFG)
	@echo end >> $(ABSDIR)/$(GDBCFG)
	@echo end >> $(ABSDIR)/$(GDBCFG)
	@echo adds init >> $(ABSDIR)/$(GDBCFG)
	@echo b main >> $(ABSDIR)/$(GDBCFG)
	@echo b $(KERNEL_ENTRY) >> $(ABSDIR)/$(GDBCFG)
	@echo c >> $(ABSDIR)/$(GDBCFG)

cleansetup:
	@sudo make clean
	@sudo make cprogs
	@sudo rm -rf $(BUILDHOME)
	@sudo rm -f $(ABSDIR)/$(GDBCFG)
	@sudo rm -rf $(LMNDIR)
	@sudo rm -rf $(PROGSBUILDDIR)
	@sudo rm -rf $(PROGSSYMDIR)

cleanup:
	@echo "Cleaning up..."
# Iterate all loop devices and detach them, find the mounted ones and unmount them
	@losetup | grep $(IMG) | awk '{print $$1}' | xargs -I {} losetup -d {}
	@mount | grep $(MNTDIR) | awk '{print $$3}' | xargs -I {} umount {}
	@rm -f $(BUILDDIR)/$(LOCKFILE)
	@make cleansetup
	@make setup

buildimggpt_second_phase:
	@echo "1" > $(BUILDDIR)/$(LOCKFILE)
	@echo "Building GPT phase 2..."
	$(eval LOOP_DEV_PATH := $(shell losetup -a | grep $(IMG) | cut -d: -f1))
	@echo $(LOOP_DEV_PATH)
	@echo "Partitioning disk..."
	@echo $(shell gcc -v)
	@losetup -l
	
	@echo "Sgdisk step"
	@sgdisk --new 1:0:+50M --typecode 1:ef00 $(LOOP_DEV_PATH)
	@sgdisk --new 2:0:+100M -t 2:8300 $(LOOP_DEV_PATH)
	@sgdisk --new 3:0:0  -t 3:8300 $(LOOP_DEV_PATH)
	@sgdisk -p $(LOOP_DEV_PATH)
	@fdisk -l $(LOOP_DEV_PATH)

	@echo "Partprobe step"
	@partprobe
	@echo "Mkfs step"
	@mkfs.fat -n PATATA -F32 $(LOOP_DEV_PATH)p1
	@mkfs.fat -n FDATA -F32 $(LOOP_DEV_PATH)p2
	@mkfs.ext2 -L FDATA2 $(LOOP_DEV_PATH)p3 -b $(BLOCKSIZE)
	@dumpe2fs $(LOOP_DEV_PATH)p3 | grep -i superblock

	@echo "Creating mount points"
	@rm -rf $(MNTDIR)
	@mkdir $(MNTDIR)
	@mkdir $(SYSDIR)
	@mkdir $(DATDIR)

	@mount $(LOOP_DEV_PATH)p1 $(SYSDIR)

	@echo Disk mounted, creating system files
	@mkdir -p $(SYSDIR)/efi/boot
	@mkdir -p $(SYSDIR)/test

	@cp $(BOOTEFI) $(SYSDIR)/efi/boot
	@cp ./startup.nsh $(SYSDIR)
	@cp $(BUILDDIR)/$(KERNEL) $(SYSDIR)
	@cp ./limine.conf $(SYSDIR)
	@umount $(SYSDIR)

	@echo System files created, creating f32 data files
	@mount $(LOOP_DEV_PATH)p2 $(DATDIR)
	@cp -r $(PROGSBUILDDIR) $(DATDIR)
	@cp -r $(PROGSDATADIR) $(DATDIR)
	@tree $(DATDIR)
	@umount $(DATDIR)

	@echo F32 data files created, creating ext2 data files
	@mount $(LOOP_DEV_PATH)p3 $(DATDIR)
	@cp -r $(PROGSBUILDDIR) $(DATDIR)
	@cp -r $(PROGSDATADIR) $(DATDIR)
	@tree $(DATDIR)
	@umount $(DATDIR)

	@rm -rf $(MNTDIR)
	@losetup -d $(LOOP_DEV_PATH)

	@echo Disk mounted, creating system files
	@rm -f $(BUILDDIR)/$(LOCKFILE)

buildimggpt:
#if $(BUILDDIR)/$(LOCKFILE), call cleanup
	@if [ -f $(BUILDDIR)/$(LOCKFILE) ]; then \
		echo "Lockfile exists, cleaning up..."; \
		make cleanup; \
	fi
	rm -f $(BUILDDIR)/$(LOCKFILE)
	@echo "Building GPT..."
	
	@make clean
	@make kernel
	@echo "Building GPT..."

	@cp $(ISODIR)/$(IMG_RAW) $(ISODIR)/$(IMG)
	@echo "Creating disk..."
	@losetup -l
	@echo "Partitioning disk..."
	@losetup -fP $(ISODIR)/$(IMG)
	@make buildimggpt_second_phase
	@chmod 666 $(ISODIR)/$(IMG)

run:
	$(QEMU) $(QFLAGSEXP)$(ISODIR)/$(IMG)

gpt:
	@make buildimggpt
	@echo "Running GPT QEMU..."
	@make run
	@echo "Done!"

debugpt:
	@make buildimggpt
	@echo "Running GPT QEMU..."
	tmux split-window -h '$(GDB) $(GDBFLAGS)' & $(QEMU) -S -s $(QFLAGSEXP)$(ISODIR)/$(IMG)

debugpt-wsl:
	@sudo make clean
	@sudo make cprogs
	@make progs
	@make kernel
	@echo "Building GPT..."
#This is required to be before buildimggpt!
# Due to eval weird behaviour
	@sudo make buildimggpt
	@echo "Running GPT QEMU..."
	cmd.exe /c start cmd /c wsl -e $(GDB) $(GDBFLAGS) & $(QEMU) -S -s $(QFLAGSEXP)$(ISODIR)/$(IMG)

debugsetup:
	@losetup -l
	@mount
	@ls -slart /mnt


progs:
	$(foreach prog,$(PROGS),$(MAKE) -C $(prog); rsync -a $(prog)/build/bin/ $(PROGSBUILDDIR);rsync -a $(prog)/build/sym/ $(PROGSSYMDIR);)

cprogs:
	$(foreach prog,$(PROGS),$(MAKE) -C $(prog) clean;)
	@rm -rf $(PROGSBUILDDIR)/*
	@rm -rf $(PROGSSYMDIR)/*

.PHONY: progs