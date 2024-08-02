#ifndef _VM_H
#define _VM_H

#if defined(ARCH) && ARCH == "x86_64"
#include <omen/hal/arch/x86/vm.h>
#include <omen/hal/arch/x86/int.h>
#include <omen/hal/arch/x86/cpu.h>
#endif

#endif