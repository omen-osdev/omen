#ifndef __MOUSE_H__
#define __MOUSE_H__
#include <omen/libraries/std/stdint.h>
#include "ps2.h"

typedef struct {
	long x;
	long y;
} Point;

#define PS2LeftButton   0b00000001
#define PS2RightButton  0b00000010
#define PS2MiddleButton 0b00000100
#define PS2XSign        0b00010000
#define PS2YSign        0b00100000
#define PS2XOverflow    0b01000000
#define PS2YOverflow    0b10000000

extern uint8_t MousePointer[];

void init_mouse(uint16_t width, uint16_t height);
uint8_t get_mouse(struct ps2_mouse_status* status);
void handle_mouse(uint8_t data);
uint8_t process_mouse_packet(struct ps2_mouse_status* status, uint8_t* packet);
uint8_t process_current_mouse_packet(struct ps2_mouse_status* status);
extern Point MousePosition;
#endif