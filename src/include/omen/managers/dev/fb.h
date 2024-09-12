#ifndef _FB_H
#define _FB_H

#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/libraries/std/stddef.h>
#include <omen/libraries/std/string.h>

void clearscreen(uint32_t);
void scroll();
void putpixel(uint64_t, uint64_t, uint32_t);
void putchar(char);
void init_framebuffer();
size_t fb_get_width();
size_t fb_get_height();

#endif
