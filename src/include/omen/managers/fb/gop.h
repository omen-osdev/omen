#ifndef _GOP_H
#define _GOP_H

#include <omen/managers/boot/bootloaders/limine/limine.h>
#include <omen/libraries/std/stddef.h>

void gop_initialize();
void gop_clear(uint32_t);
void gop_pixel(uint64_t, uint64_t, uint32_t);
void gop_putchar(char);

#endif
