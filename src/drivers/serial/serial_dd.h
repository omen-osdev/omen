#ifndef _SERIAL_DD_H
#define _SERIAL_DD_H
#include <omen/libraries/std/stdint.h>
#define SERIAL_FLUSH_TX             0x1
#define SERIAL_SUBSCRIBE_READ       0x2
#define SERIAL_SUBSCRIBE_WRITE      0x3
#define SERIAL_UNSUBSCRIBE_READ     0x4
#define SERIAL_UNSUBSCRIBE_WRITE    0x5
#define SERIAL_ENABLE_ECHO          0x6
#define SERIAL_DISABLE_ECHO         0x7
#define SERIAL_DISCARD              0x8
#define SERIAL_GET_CONFIG           0x9
#define SERIAL_SET_CONFIG           0xa

#define DEVICE_SERIAL               0x8d
#define SERIAL_DD_NAME "SERIAL DRIVER\0"

struct serial_ioctl_subscriptor {
    void* parent;
    void (*handler)(void * parent, char c, int port);
} __attribute__((packed));

struct serial_ioctl_configuration {
    int baud_rate;
    int parity;
    int stop_bits;
    int data_bits;
};

void init_serial_dd();

#endif