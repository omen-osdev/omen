#if !defined(__GNUC__)
#error "GCC required"
#else
#ifndef _GETCPUID_H
#define _GETCPUID_H
//Check if we are using gcc compiler

int get_model(void);

int get_cpu_vendor(void);
 
int check_apic(void);

int check_msr(void);

int get_maxphyaddr(void);

void enable_cpuid(unsigned int enabled);

#endif
#endif