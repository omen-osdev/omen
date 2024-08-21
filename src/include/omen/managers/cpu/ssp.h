#ifndef _STACK_SMASHING_PROTECTION_H
#define _STACK_SMASHING_PROTECTION_H

#include <omen/libraries/std/stdint.h>
#include <omen/apps/panic/panic.h>

#if UINT32_MAX == UINTPTR_MAX
#define STACK_CHK_GUARD 0xe2dee396
#else
#define STACK_CHK_GUARD 0x595e9fbd94fda766
#endif
 
__attribute__((noreturn)) void __stack_chk_fail(void);

#endif
