#ifndef _PERCPU_H
#define _PERCPU_H

#define PER_CPU_BASE_SECTION ".data..percpu"

#define DEFINE_PER_CPU(type, name) DEFINE_PER_CPU_SECTION(type, name, "")

#define PER_CPU_ATTRIBUTES __attribute__((aligned(4096)))

#define __PCPU_ATTRS(sec)                                                \
         __percpu __attribute__((section(PER_CPU_BASE_SECTION sec)))     \
         PER_CPU_ATTRIBUTES

#define DEFINE_PER_CPU_SECTION(type, name, sec)    \
         __PCPU_ATTRS(sec) PER_CPU_DEF_ATTRIBUTES  \
         __typeof__(type) name
    
#endif