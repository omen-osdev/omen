#ifndef _X86_IO_H
#define _X86_IO_H

#include <omen/libraries/std/stdint.h>

static inline uint8_t inb(uint16_t port)
{
    uint8_t data;
    __asm__ volatile ( "inb %w1, %b0" : "=a"(data) : "Nd"(port) : "memory");
    return data;
}

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ( "outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t data;
    __asm__ volatile ( "inw %w1, %w0" : "=a"(data) : "Nd"(port) : "memory");
    return data;
}

static inline void outw(uint16_t port, uint16_t val)
{
    __asm__ volatile ( "outw %w0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint32_t inl(uint16_t port)
{
    uint32_t data;
    __asm__ volatile ( "inl %w1, %0" : "=a"(data) : "Nd"(port) : "memory");
    return data;
}

static inline void outl(uint16_t port, uint32_t val)
{
    __asm__ volatile ( "outl %0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline void insw(uint16_t port, void * addr, uint32_t count)
{
    __asm__ volatile ( "cld; rep insw" : "+D"(addr), "+c"(count) : "d"(port) : "memory", "cc");
}

static inline void outsw(uint16_t port, void * addr, uint32_t count)
{
    __asm__ volatile ( "cld; rep outsw" : "+S"(addr), "+c"(count) : "d"(port) : "memory", "cc");
}

static inline void io_wait(void)
{
    __asm__ volatile ( "outb %%al, $0x80" : : "a"(0) );
}

#endif