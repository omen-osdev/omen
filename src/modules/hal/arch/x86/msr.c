#include <omen/hal/arch/x86/msr.h>
 
void cpuGetMSR(uint32_t msr, uint32_t *lo, uint32_t *hi) {
   __asm__ volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}
 
void cpuSetMSR(uint32_t msr, uint32_t lo, uint32_t hi) {
   __asm__ volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

void cpu_set_msr_u64(uint32_t msr, uint64_t value) {
    cpuSetMSR(msr, (uint32_t) value, (uint32_t) (value >> 32));
}

uint64_t cpu_get_msr_u64(uint32_t msr) {
      uint32_t lo, hi;
      cpuGetMSR(msr, &lo, &hi);
      return ((uint64_t) hi << 32) | lo;
}