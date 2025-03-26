#ifndef _SERIAL_INTERFACE_H
#define _SERIAL_INTERFACE_H
#include <omen/libraries/std/stdint.h>
uint64_t serial_read(const char * device, uint8_t * buffer, uint32_t skip, uint32_t size);
uint64_t serial_read_echo(const char * device, uint8_t * buffer, uint32_t skip, uint32_t size);
uint64_t serial_write(const char * device, uint8_t * buffer, uint32_t skip, uint32_t size);
uint64_t serial_write_now(const char * device, const char * buffer, uint32_t skip, uint32_t size);
uint64_t serial_ioctl(const char * device, uint32_t op, void * buffer);
uint64_t serial_identify(const char * device);
#endif