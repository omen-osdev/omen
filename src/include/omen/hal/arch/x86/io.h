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

#endif