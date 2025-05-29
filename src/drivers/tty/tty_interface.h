#ifndef _TTY_INTERFACES_H
#define _TTY_INTERFACES_H

#define TTY_SIGNAL_OUTB 0
#define TTY_SIGNAL_INB 1
#define TTY_SIGNAL_FLUSH_INB 2
#define TTY_SIGNAL_FLUSH_OUTB 3

#include <omen/libraries/std/string.h>
#include "tty_dd.h"

char * create_tty(char* in, char* out, int mode, int inbs, int outbs);
void destroy_tty(const char * device);

uint64_t tty_read(const char * device, uint8_t * buffer, uint32_t skip, uint32_t size);
uint64_t tty_write(const char * device, uint8_t * buffer, uint32_t skip, uint32_t size);
uint64_t tty_write_now(const char * device, const char * buffer, uint32_t skip, uint32_t size);
uint64_t tty_ioctl(const char * device, uint32_t op, void * buffer);
uint64_t tty_identify(const char* device);
uint64_t tty_add_subscriber(const char* device, void (*subscriber)(void*, uint8_t));
uint64_t tty_remove_subscriber(const char* device, void (*subscriber)(void*, uint8_t));
uint64_t tty_flush(const char* device);
uint64_t tty_set_echo(const char* device, uint8_t echo);
uint64_t tty_set_raw(const char* device, uint8_t raw);
uint64_t tty_get_size(const char* device);
uint64_t tty_has_input(const char* device);
uint64_t tty_validate(const char* device);

uint64_t tty_read_direct(struct tty* tty, uint8_t * buffer, uint32_t skip, uint32_t size);
uint64_t tty_write_direct(struct tty* tty, uint8_t * buffer, uint32_t skip, uint32_t size);
#endif