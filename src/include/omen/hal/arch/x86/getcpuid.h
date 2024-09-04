#if !defined(__GNUC__)
#error "GCC required"
#else
#ifndef _GETCPUID_H
#define _GETCPUID_H
//Check if we are using gcc compiler

int get_model(void);
 
int check_apic(void);

int check_msr(void);

#endif
#endif