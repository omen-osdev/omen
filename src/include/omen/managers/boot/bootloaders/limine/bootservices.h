#ifndef _LIMINE_BOOTSERVICES_H
#define _LIMINE_BOOTSERVICES_H

#include <omen/managers/boot/bootloaders/abstract.h>
#include <omen/managers/boot/bootloaders/limine/limine.h>

struct bootloader_operations * get_boot_ops();

#endif