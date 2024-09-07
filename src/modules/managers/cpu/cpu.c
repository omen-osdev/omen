#include <omen/managers/cpu/cpu.h> 
#include <omen/hal/hal.h>

void init_cpus() {
    arch_init_fpu();
    arch_init_cpu();
}

void init_simd() {
    arch_init_simd();
}

void enable_cpu(uint64_t cpu_index) {
    arch_set_alive(cpu_index, 1);
}

void disable_cpu(uint64_t cpu_index) {
    arch_set_alive(cpu_index, 0);
}
uint64_t get_cpu_count();
boot_smp_info_t * get_specific_cpu(uint64_t index);
boot_smp_info_t * get_bsp();
uint64_t get_bsp_index();