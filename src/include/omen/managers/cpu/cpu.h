#ifndef _CPU_H
#define _CPU_H

#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/libraries/std/stdint.h>
#include <omen/hal/arch/x86/cpu.h>

void init_cpus();
void init_simd();
void enable_cpu(uint64_t cpu_index);
void disable_cpu(uint64_t cpu_index);
uint64_t get_cpu_count();
boot_smp_info_t * get_specific_cpu(uint64_t index);
boot_smp_info_t * get_bsp();
uint64_t get_bsp_index();
#endif