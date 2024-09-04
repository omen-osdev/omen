#ifndef _CAPABILITIES_H
#define _CAPABILITIES_H
#include <omen/libraries/std/stdint.h>
extern uint64_t getCr0();
extern uint64_t getCr2();
extern uint64_t getCr3();
extern uint64_t getCr4();
extern uint64_t getCr8();
extern uint64_t getEfer();
extern uint64_t getFsBase();
extern uint64_t getGsBase();
extern uint64_t getKernelGsBase();
extern uint64_t getRflags();
extern uint8_t  getApicId();
extern void setRflags(uint64_t flags);
#endif