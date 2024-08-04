override KERNEL := kernel.elf
override ISO := limine-cd.iso
override IMG := limine-cd.img
override VDI := limine-cd.vdi
override IMG_RAW := raw.img
override LIMINECFG := limine.cfg
override GDBCFG := debug.gdb
override VMNAME := omen
override WDIR := omendata

# It is highly recommended to use a custom built cross toolchain to build a kernel.
# We are only using "cc" as a placeholder here. It may work by using
# the host system's toolchain, but this is not guaranteed.

CC := /usr/bin/cc
LD := /usr/bin/ld
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
CMDNEWSCREEN := cmd.exe /c start cmd /c wsl -e
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

CFLAGS ?= -O3 -g -Wall -Wextra -pipe -Wno-packed-bitfield-compat -std=c11
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
	$(eval WINUSER := $(shell cmd.exe /c 'for %a in (%userprofile%) do (echo %~nxa)' | tail -1 | tr -d ' '))
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(ISOBUILDDIR)
	@mkdir -p $(ISODIR)
	@mkdir -p /mnt/c/Users/$(WINUSER)/$(WDIR)
	@dd if=/dev/zero of=$(ISODIR)/$(IMG_RAW) bs=4096 count=102400
	@git clone $(LMNREPO) --branch=$(LMNBRCH) --depth=1
	@cp -v $(LMNDIR)/limine.sys $(LMNDIR)/limine-cd.bin $(LMNDIR)/limine-cd-efi.bin $(ISOBUILDDIR)
	@cp $(OVMFDIR)/OVMF_CODE-pure-efi.fd /mnt/c/Users/$(WINUSER)/$(WDIR)/OVMF_CODE-pure-efi.fd
	@cp $(OVMFDIR)/OVMF_VARS-pure-efi.fd /mnt/c/Users/$(WINUSER)/$(WDIR)/OVMF_VARS-pure-efi.fd
	@echo file $(BUILDDIR)/$(KERNEL) > debug.gdb
	@echo target remote $(WSLHOSTIP):$(GDBPORT) >> debug.gdb
	@echo set disassembly-flavor intel >> debug.gdb
	@echo b $(KERNEL_ENTRY) >> debug.gdb
	@echo b interrupt_exception_handler >> debug.gdb
	@echo b PageFault_Handler >> debug.gdb
	@echo b DoubleFault_Handler >> debug.gdb
	@echo b GPFault_Handler >> debug.gdb
	@echo c >> debug.gdb

cleansetup:
	$(eval WINUSER := $(shell cmd.exe /c 'for %a in (%userprofile%) do (echo %~nxa)' | tail -1 | tr -d ' '))
	@rm -rf $(BUILDHOME)
	@rm -f debug.gdb
	@rm -rf $(LMNDIR)
	@rm -rf /mnt/c/Users/$(WINUSER)/$(WDIR)
buildimg:
	@cp -v limine.cfg $(BUILDDIR)/$(KERNEL) $(ISOBUILDDIR)
	@xorriso -as mkisofs -b limine-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table \
        --efi-boot limine-cd-efi.bin \
        -efi-boot-part --efi-boot-image --protective-msdos-label \
        $(ISOBUILDDIR) -o $(ISODIR)/$(ISO)
	$(LMN) $(ISODIR)/$(ISO)

buildimgexp:
	@cp $(ISODIR)/$(IMG_RAW) $(ISODIR)/$(IMG)
	@mkfs.fat -F32 $(ISODIR)/$(IMG)
	@mmd -i $(ISODIR)/$(IMG) ::/EFI
	@mmd -i $(ISODIR)/$(IMG) ::/EFI/BOOT
	@mcopy -i $(ISODIR)/$(IMG) $(BOOTEFI) ::/EFI/BOOT
	@mcopy -i $(ISODIR)/$(IMG) ./startup.nsh ::
	@mcopy -i $(ISODIR)/$(IMG) $(BUILDDIR)/$(KERNEL) ::
	@mcopy -i $(ISODIR)/$(IMG) ./limine.cfg ::

buildimggpt:
	@gcc -v
	$(eval LOOP_DEV_PATH := $(shell losetup -a | grep $(IMG) | cut -d: -f1))
	@echo Loop device path: $(LOOP_DEV_PATH)
	
	@sudo sgdisk --new 1:0:+50M --typecode 1:ef00 $(LOOP_DEV_PATH)
	@sudo sgdisk --new 2:0:+100M -t 2:8300 $(LOOP_DEV_PATH)
	@sudo sgdisk --new 3:0:0  -t 3:8300 $(LOOP_DEV_PATH)
	@sudo sgdisk -p $(LOOP_DEV_PATH)
	@sudo fdisk -l $(LOOP_DEV_PATH)
	@-sudo partprobe
	@sudo mkfs.fat -n PATATA -F32 $(LOOP_DEV_PATH)p1
	@sudo mkfs.fat -n FDATA -F32 $(LOOP_DEV_PATH)p2
	@sudo mkfs.ext2 -L FDATA2 $(LOOP_DEV_PATH)p3 -b $(BLOCKSIZE)
	@sudo dumpe2fs $(LOOP_DEV_PATH)p3 | grep -i superblock
	@sudo mkdir $(MNTDIR)
	@sudo mkdir $(SYSDIR)
	@sudo mkdir $(DATDIR)

	@sudo mount $(LOOP_DEV_PATH)p1 $(SYSDIR)

	@echo Disk mounted, creating system files
	@sudo mkdir -p $(SYSDIR)/efi/boot
	@sudo cp $(BOOTEFI) $(SYSDIR)/efi/boot
	@sudo cp ./startup.nsh $(SYSDIR)
	@sudo cp $(BUILDDIR)/$(KERNEL) $(SYSDIR)
	@sudo cp ./limine.cfg $(SYSDIR)
	@sudo umount $(SYSDIR)

	@echo System files created, creating f32 data files
	@sudo mount $(LOOP_DEV_PATH)p2 $(DATDIR)
	@sudo tree $(DATDIR)
	@sudo umount $(DATDIR)

	@echo F32 data files created, creating ext2 data files
	@sudo mount $(LOOP_DEV_PATH)p3 $(DATDIR)
	@sudo tree $(DATDIR)
	@sudo umount $(DATDIR)

	@sudo rm -rf $(MNTDIR)
	@sudo losetup -d $(LOOP_DEV_PATH)

run:
	$(QEMU) $(QFLAGS) $(ISODIR)/$(ISO)

#run_exp:
#	$(eval WINUSER := $(shell cmd.exe /c 'for %a in (%userprofile%) do (echo %~nxa)' | tail -1 | tr -d ' '))
#	$(eval WINDIRECTORY := C:/Users/$(WINUSER)/$(WDIR))
#	@echo "WINDIRECTORY: $(WINDIRECTORY)"
#	@echo "Copying files to Windows directory..."
#	@cp $(ISODIR)/$(IMG) /mnt/c/Users/$(WINUSER)/$(WDIR)/$(IMG)
#	@ls -l /mnt/c/Users/$(WINUSER)/$(WDIR)
#	$(QEMU) $(QWFLAGSEXP)$(WINDIRECTORY)/$(IMG)

run_exp:
	$(QEMU) $(QFLAGSEXP)$(ISODIR)/$(IMG)

gpt:
	@make clean
	@make kernel
	@echo "Building GPT..."
#This is required to be before buildimggpt!
	@cp $(ISODIR)/$(IMG_RAW) $(ISODIR)/$(IMG)
	@sudo losetup -fP $(ISODIR)/$(IMG)
# Due to eval weird behaviour
	@make buildimggpt
	@echo "Running GPT QEMU..."
	@make run_exp
	@echo "Done!"

exp:
	@make kernel
	@echo "Building experimental ISO..."
	@make buildimgexp
	@echo "Running experimental QEMU..."
	@make run_exp
	@echo "Done!"

#debugpt:
#	$(eval WINUSER := $(shell cmd.exe /c 'for %a in (%userprofile%) do (echo %~nxa)' | tail -1 | tr -d ' '))
#	$(eval WINDIRECTORY := C:/Users/$(WINUSER)/$(WDIR))
#	@make kernel
#	@echo "Building GPT..."
##This is required to be before buildimggpt!
#	@cp $(ISODIR)/$(IMG_RAW) $(ISODIR)/$(IMG)
#	@sudo losetup -Pf $(ISODIR)/$(IMG)
## Due to eval weird behaviour
#	@make buildimggpt
#	@echo "Running GPT QEMU..."
#	@echo "WINDIRECTORY: $(WINDIRECTORY)"
#	@echo "Copying files to Windows directory..."
#	@cp $(ISODIR)/$(IMG) /mnt/c/Users/$(WINUSER)/$(WDIR)/$(IMG)
#	@ls -l /mnt/c/Users/$(WINUSER)/$(WDIR)
#	$(CMDNEWSCREEN) $(GDB) $(GDBFLAGS) & $(QEMU) -S -s $(QWFLAGSEXP)$(WINDIRECTORY)/$(IMG)

debugpt:
	@make clean
	@make kernel
	@echo "Building GPT..."
#This is required to be before buildimggpt!
	@cp $(ISODIR)/$(IMG_RAW) $(ISODIR)/$(IMG)
	@sudo losetup -Pf $(ISODIR)/$(IMG)
# Due to eval weird behaviour
	@make buildimggpt
	@echo "Running GPT QEMU..."
	$(CMDNEWSCREEN) $(GDB) $(GDBFLAGS) & $(QEMU) -S -s $(QFLAGSEXP)$(ISODIR)/$(IMG)

debuge:
	@make kernel
	@echo "Building experimental ISO..."
	@make buildimgexp
	@echo "Running experimental QEMU..."
	$(QEMU) -S -s $(QFLAGSEXP)$(ISODIR)/$(IMG)

debug:
	@make kernel
	@make buildimg
	$(CMDNEWSCREEN) $(GDB) $(GDBFLAGS) &
	$(QEMU) -S -s $(QFLAGS) $(ISODIR)/$(ISO)

netconfig:
	brctl addbr br0
	