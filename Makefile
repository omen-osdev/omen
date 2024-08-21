CC := gcc
AS := as
LD := ld

CFLAGS := -O0 \
-ffreestanding \
-nostdlib \
-nostartfiles \
-fno-stack-protector \
-fno-stack-check \
-fno-lto \
-mno-red-zone \
-m64 \
-g \
-pipe \
-Wno-packed-bitfield-compat \
-Wall \
-Wextra \
-std=c11 \
-x c \
-Isrc/include \
-Isrc/conf \
-Isrc/drivers

LDFLAGS := -m elf_x86_64 \
-nostdlib \
-static \
-z max-page-size=0x1000

src_cc := ${shell find src/ -type f -iname "*.c"}
src_as := ${shell find src/ -type f -iname "*.s"}
obj_cc := ${patsubst src/%.c,bin/%.cc_o,${src_cc}}
obj_as := ${patsubst src/%.s,bin/%.as_o,${src_as}}

image := bin/image.img
kernel := bin/kernel.elf

.PHONY:all
all:${image}

${image}:${kernel}
	dd if=/dev/zero of=$@ bs=512 count=131072
	mkfs.fat -F 32 $@
	mmd -i $@ ::/EFI ::EFI/BOOT
	mcopy -i $@ limine/BOOTX64.EFI ::/EFI/BOOT
	mcopy -i $@ limine/limine.conf ::/
	mcopy -i $@ $< ::/

${kernel}:${obj_as} ${obj_cc}
	@${LD} ${LDFLAGS} -Tsrc/linker.ld $^ -o $@

bin/%.cc_o:src/%.c
	@mkdir -p ${dir $@}
	@${CC} ${CFLAGS} -c $< -o $@

bin/%.as_o:src/%.s
	@mkdir -p ${dir $@}
	@${AS} $< -o $@

.PHONY:clean
clean:
	@rm bin -rf
