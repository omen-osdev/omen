#ifndef _PS2_H
#define _PS2_H

#include <omen/libraries/std/stdint.h>

#define DEVICE_MOUSE               0x10
#define DEVICE_KEYBOARD            0x8f
#define MOUSE_DD_NAME "PS2 MOUSE DRIVER\0"
#define KEYBOARD_DD_NAME "PS2 KEYBOARD DRIVER\0"

#define PS2_DEVICE_MOUSE    0x00
#define PS2_DEVICE_KEYBOARD 0x01

#define PS2_DEVICE_GENERIC_EVENT  0x00
#define PS2_DEVICE_SPECIAL_EVENT  0x01

#define MOUSE_X_OVERFLOW    0b01000000
#define MOUSE_Y_OVERFLOW    0b10000000
#define MOUSE_LEFT_CLICK    0b00000001
#define MOUSE_RIGHT_CLICK   0b00000010
#define MOUSE_MIDDLE_CLICK  0b00000100

#define IOCTL_MOUSE_SUBSCRIBE           0x01
#define IOCTL_MOUSE_UNSUBSCRIBE         0x02
#define IOCTL_MOUSE_SUBSCRIBE_EVENT     0x03
#define IOCTL_MOUSE_GET_STATUS          0x04

#define IOCTL_KEYBOARD_SUBSCRIBE            0x01
#define IOCTL_KEYBOARD_UNSUBSCRIBE          0x02
#define IOCTL_KEYBOARD_HALT_UNTIL_ENTER     0x03

struct ps2_kbd_ioctl_subscriptor {
    void* parent;
    void (*handler)(void* data, char c, int ignore);
};

struct ps2_mouse_status {
    uint16_t x;
    uint16_t y;
    uint8_t buttons; // 0b00000LMR
    uint8_t scroll;
    uint8_t flags;
};

void init_ps2_dd();
uint8_t read_mouse(struct ps2_mouse_status * mouse);
uint8_t read_keyboard(char * key);
void keyboard_halt_until_enter();
void ps2_subscribe(void* handler, uint8_t device, uint8_t event);
void ps2_unsubscribe(void *handler);

#endif