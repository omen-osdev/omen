// OMEN PS/2 driver

#ifndef _PS2_DRIVER_H
#define _PS2_DRIVER_H

#include "../../include/omen/libraries/std/stdint.h"

#define DEVICE_PS2 0x8e
#define PS2_DD_NAME "ps2"

char* create_ps2_dd();
status_t init_ps2_dd();
uint64_t ps2_dd_ioctl(uint64_t id, uint32_t request, void* data);
uint64_t ps2_dd_rcvd();
uint64_t ps2_dd_read(uint64_t id, uint8_t* buffer);

#endif