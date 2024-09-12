#ifndef _KEYBOARD_H
#define _KEYBOARD_H
#include <omen/libraries/std/stdint.h>
#define ASCII_SIZE 56

#define BUFFER_SIZE 1024

#define LeftShift 0x2A
#define RightShift 0x36
#define Enter 0x1C
#define Backspace 0x0E
#define Spacebar 0x39
#define KEYBOARD_IRQ 0x21

struct keyboard {
    char * ASCII_table;
    uint8_t left_shift_pressed;
    uint8_t right_shift_pressed;
    uint8_t intro_buffered;
};

void init_keyboard();
char get_last_key();
char handle_keyboard(uint8_t);
void halt_until_enter();
#endif