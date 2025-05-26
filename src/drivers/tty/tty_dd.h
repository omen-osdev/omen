#ifndef _TTY_DD_H
#define _TTY_DD_H

#include <omen/libraries/std/stdint.h>
#include "tty.h"

#define DEVICE_TTY 0xe

#define TTY_ADD_SUBSCRIBER 0x1
#define TTY_REMOVE_SUBSCRIBER 0x2
#define TTY_MODE_RAW 0x3
#define TTY_MODE_ECHO 0x4
#define TTY_FLUSH 0x5
#define TTY_VALIDATE 0x6
#define TTY_GET_SIZE 0x7
#define TTY_INB_TO_OUTB 0x8
#define TTY_GET_SERIAL_SETTINGS               0x5401
#define TTY_SET_SERIAL_SETTINGS               0x5402
#define TTY_SET_SERIAL_SETTINGS_WITHOUT_FLUSH 0x5403
#define TTY_SET_SERIAL_SETTINGS_WITH_FLUSH    0x5404
#define TTY_GWINSZ                            0x5413

//#define TIOCSCTTY 0x540E
//#define TIOCGPGRP 0x540F
//#define TIOCSPGRP 0x5410
//#define TIOCGWINSZ 0x5413
//#define TIOCSWINSZ 0x5414
//#define TIOCGSID 0x5429

#define TTY_CHECK_VAL 0x69
#define TTY_DD_NAME "TTY DRIVER\0"

void init_tty_dd();
uint64_t tty_dd_read_block_direct(struct tty* dev, uint64_t size, uint64_t skip, uint8_t* buffer);
uint64_t tty_dd_write_block_direct(struct tty* dev, uint64_t size, uint64_t skip, uint8_t* buffer);
#endif