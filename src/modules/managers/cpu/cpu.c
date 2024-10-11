#include <omen/managers/cpu/cpu.h> 
#include <omen/hal/hal.h>

void init_cpus() {
    arch_init_fpu();
    arch_init_cpu();
}

void init_simd() {
    arch_init_simd();
}

uint64_t get_cpu_count();
boot_smp_info_t * get_specific_cpu(uint64_t index);
boot_smp_info_t * get_bsp();
uint64_t get_bsp_index();