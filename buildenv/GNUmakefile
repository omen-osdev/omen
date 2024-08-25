override KERNEL := kernel.elf
override ISO := limine-cd.iso
override IMG := limine-cd.img
override VDI := limine-cd.vdi
override IMG_RAW := raw.img
override LIMINECFG := limine.cfg
override GDBCFG := debug.gdb
override VMNAME := omen
override WDIR := omendata
override LOCKFILE := lockfile

# It is highly recommended to use a custom built cross toolchain to build a kernel.
# We are only using "cc" as a placeholder here. It may work by using
# the host system's toolchain, but this is not guaranteed.

TARGET = x86_64-elf-
CC := $(TARGET)gcc
LD := $(TARGET)ld
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
VBOXMANAGE := "/mnt/c/Program Files/Oracle/VirtualBox/VBoxManage.exe"
VBOXHEADLESS := "/mnt/c/Program Files/Oracle/VirtualBox/VBoxHeadless.exe"
VBOXCOM1PORT := 4441
VBOXCOM2PORT := 4442
VBOXCOM3PORT := 4443
VBOXCOM4PORT := 4444
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

QFLAGS ?= -cpu qemu64 -d cpu_reset -no-reboot -no-shutdown -machine q35 $(NUMAFLAGS) -m $(MEMSIZE) -boot d -serial stdio -serial telnet::4444,server,nowait -cdrom 
QFLAGSEXP ?= -cpu qemu64 -d cpu_reset -no-reboot -no-shutdown -machine q35 $(NUMAFLAGS) -m $(MEMSIZE) -boot d -serial stdio -drive if=pflash,format=raw,unit=0,file=$(OVMFDIR)/OVMF_CODE-pure-efi.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=$(OVMFDIR)/OVMF_VARS-pure-efi.fd -drive file=
#QWFLAGSEXP ?= -cpu qemu64 -d cpu_reset -machine q35 $(NUMAFLAGS) -m $(MEMSIZE) -boot d -netdev tap,id=mynet0,ifname=tap,script=no,downscript=no -serial stdio -device e1000,netdev=mynet0,mac=51:52:53:54:55:56 -drive if=pflash,format=raw,unit=0,file=$(WINDIRECTORY)/OVMF_CODE-pure-efi.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=$(WINDIRECTORY)/OVMF_VARS-pure-efi.fd -drive file=

CFLAGS ?= -O0 -g -Wall -Wextra -pipe -Wno-packed-bitfield-compat -std=c11
NASMFLAGS ?= -F dwarf -g
LDFLAGS ?=
CPPFLAGS ?=
GDBFLAGS ?=  --nx --command=$(ABSDIR)/$(GDBCFG)
GDBPORT ?= 1234

ABSDIR := $(shell pwd)/..
LMNDIR := $(shell pwd)/limine
OVMFDIR := $(shell pwd)/OVMFbin
SRCDIR := $(ABSDIR)/src
INCDIR := $(SRCDIR)/include
CONFDIR := $(SRCDIR)/conf
DRVDIR := $(SRCDIR)/drivers
BUILDHOME := $(ABSDIR)/build
BUILDDIR := $(ABSDIR)/build/bin
OBJDIR := $(ABSDIR)/build/lib
ISOBUILDDIR := $(ABSDIR)/build/iso_root
ISODIR := $(ABSDIR)/build/image
# /mnt/omen
SYSDIR := $(MNTDIR)/system
DATDIR := $(MNTDIR)/data

LMN := $(LMNDIR)/limine-deploy.exe
BOOTEFI := $(LMNDIR)/BOOTX64.EFI

LMNREPO := https://github.com/limine-bootloader/limine.git
LMNBRCH := v3.0-branch-binary

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
    -std=c11             \
    -ffreestanding       \
    -fno-pie             \
    -fno-pic             \
    -m64                 \
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
	@sudo apt update
	@sudo apt install git make build-essential nasm parted dosfstools tree qemu-system gdb

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

vbon:
	@make clean
	@make kernel
	@echo "Creating iso"
	@cp $(ISODIR)/$(IMG_RAW) $(ISODIR)/$(IMG)
	@sudo losetup -f $(ISODIR)/$(IMG)
	@make buildimggpt
	@echo "translating img to vdi"
	@$(VBOXMANAGE) convertfromraw $(ISODIR)/$(IMG) $(ISODIR)/$(VDI) --format VDI
	@echo "Creating virtual machine..."
	@$(VBOXMANAGE) createvm --name $(VMNAME) --ostype "Other_64" --register
	@echo "Setting up virtual machine..."
	@$(VBOXMANAGE) modifyvm $(VMNAME) --memory $(MEMSIZE) --vram $(VMEMSIZE) --chipset ich9 --firmware efi
	@$(VBOXMANAGE) modifyvm $(VMNAME) --uart1 0x3F8 1 --uartmode1 tcpserver $(VBOXCOM1PORT) --uarttype1 16450
	@$(VBOXMANAGE) modifyvm $(VMNAME) --nic1 nat
	@$(VBOXMANAGE) storagectl $(VMNAME) --name "SATA Controller" --add sata --controller IntelAhci
	@$(VBOXMANAGE) storageattach $(VMNAME) --storagectl "SATA Controller" --port 0 --device 0 --type hdd --medium $(ISODIR)/$(VDI)
	@$(VBOXMANAGE) modifyvm $(VMNAME) --boot1 disk --boot2 none --boot3 none --boot4 none
	@echo "Starting virtual machine..."
	@$(VBOXMANAGE) startvm $(VMNAME)
	@echo "Done!"

vboff:
	@echo "Stopping virtual machine..."

	@if [ -z "$(shell $(VBOXMANAGE) list runningvms | grep $(VMNAME))" ]; then \
		echo "Virtual machine is not running!"; \
	else \
		$(VBOXMANAGE) controlvm $(VMNAME) poweroff; \
	fi
	@sleep 3
	@echo "Deleting virtual machine..."
	@$(VBOXMANAGE) unregistervm $(VMNAME) --delete
	@rm -f $(ISODIR)/$(VDI)

kernel: $(OBJS) link

$(OBJDIR)/io/interrupts.o: $(SRCDIR)/io/interrupts.c
#	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	@$(CC) $(CFLAGS) -mgeneral-regs-only -c $^ -o $@

$(OBJDIR)/drivers/serial/serial.o: $(SRCDIR)/drivers/serial/serial.c
#	@ echo !==== COMPILING $^ with -mgeneral-regs-only
	@ mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $^ -o $@

$(OBJDIR)/util/json.o: $(SRCDIR)/util/json.c
#	@ echo !==== COMPILING $^ with -mgeneral-regs-only
	@ mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $^ -o $@

$(OBJDIR)/util/dbgprinter.o: $(SRCDIR)/util/dbgprinter.c
#	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $^ -o $@

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
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(ISOBUILDDIR)
	@mkdir -p $(ISODIR)
	@dd if=/dev/zero of=$(ISODIR)/$(IMG_RAW) bs=4096 count=102400
	@git clone $(LMNREPO) --branch=$(LMNBRCH) --depth=1
	@cp -v $(LMNDIR)/limine.sys $(LMNDIR)/limine-cd.bin $(LMNDIR)/limine-cd-efi.bin $(ISOBUILDDIR)
	@echo file $(BUILDDIR)/$(KERNEL) > $(ABSDIR)/$(GDBCFG)
	@echo target remote $(WSLHOSTIP):$(GDBPORT) >> $(ABSDIR)/$(GDBCFG)
	@echo set disassembly-flavor intel >> $(ABSDIR)/$(GDBCFG)
	@echo b $(KERNEL_ENTRY) >> $(ABSDIR)/$(GDBCFG)
	@echo c >> $(ABSDIR)/$(GDBCFG)

cleansetup:
	@rm -rf $(BUILDHOME)
	@rm -f $(ABSDIR)/$(GDBCFG)
	@rm -rf $(LMNDIR)

cleanup_loop:
	@if [ -n "$(shell losetup -a | grep $(IMG))" ]; then \
		echo "There are more loop devices, cleaning up..."; \
		$(eval LOOP_DEV_PATH := $(shell losetup -a | grep $(IMG) | cut -d: -f1)) \
		sudo losetup -d $(LOOP_DEV_PATH); \
		make cleanup_loop; \
	fi

cleanup:
	@echo "Cleaning up..."
	@make cleanup_loop
	@make clean
	@make cleansetup
	@rm -f $(BUILDDIR)/$(LOCKFILE)
	@make setup

buildimggpt_second_phase:
	echo "1" > $(BUILDDIR)/$(LOCKFILE)
	echo "Building GPT..."
	echo $(shell gcc -v)
	
	truncate -s 100M $(ISODIR)/$(IMG)
	rm -rf .mtoolsrc
	echo "drive i: file=\"$(ISODIR)/$(IMG)\"" >> .mtoolsrc
	mformat -F -i $(ISODIR)/$(IMG) ::
	mdir -i $(ISODIR)/$(IMG)

	mmd -i $(ISODIR)/$(IMG) ::/efi
	mmd -i $(ISODIR)/$(IMG) ::/efi/boot

	mcopy -i $(ISODIR)/$(IMG) $(BOOTEFI) ::/efi/boot
	mcopy -i $(ISODIR)/$(IMG) ./startup.nsh ::
	mcopy -i $(ISODIR)/$(IMG) $(BUILDDIR)/$(KERNEL) ::
	mcopy -i $(ISODIR)/$(IMG) ./limine.cfg ::

	echo Disk mounted, creating system files
	rm -f $(BUILDDIR)/$(LOCKFILE)

buildimggpt:
#if process.txt exists, call cleanup, else call buildimggpt_second_phase
	if [ -f $(BUILDDIR)/$(LOCKFILE) ]; then \
		echo "Lockfile exists, cleaning up..."; \
		make cleanup; \
	fi
	
	make clean
	make kernel
	echo "Building GPT..."

	cp $(ISODIR)/$(IMG_RAW) $(ISODIR)/$(IMG)
	make buildimggpt_second_phase;

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
	@make clean
	@make kernel
	@echo "Building GPT..."
#This is required to be before buildimggpt!
# Due to eval weird behaviour
	@make buildimggpt
	@echo "Running GPT QEMU..."
	cmd.exe /c start cmd /c wsl -e $(GDB) $(GDBFLAGS) & $(QEMU) -S -s $(QFLAGSEXP)$(ISODIR)/$(IMG)

debugsetup:
	@sudo losetup -l
	@sudo mount
	@sudo ls -slart /mnt