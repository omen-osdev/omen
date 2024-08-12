#ifndef _HAL_H
#define _HAL_H

//TODO: This is probably not the best approach!
#if defined(ARCH) && ARCH == x86_64
#include <omen/hal/arch/x86/vm.h>
//TODO: FIX A WEIRD BUG HERE
//#include <omen/hal/arch/x86/int.h>
#include <omen/hal/arch/x86/cpu.h>
#endif

#endif