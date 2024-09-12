#include <omen/libraries/std/stdint.h>
#include <omen/hal/arch/x86/io.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include "mouse.h"
#include "ps2.h"

struct ps2_rect {
    uint16_t width;
    uint16_t height;
};

struct ps2_rect screen = {0, 0};
uint8_t MouseCycle = 0;
uint8_t MousePacket[4];
struct ps2_mouse_status LastMouseStatus;
uint8_t MousePacketReady = 0;

void MouseWait() {
    uint64_t timeout = 100000;
    while(timeout--) {
        if((inb(0x64) & 0b10) == 0) {
            return;
        }
    }
}

void MouseWaitInput() {
    uint64_t timeout = 100000;
    while(timeout--) {
        if(inb(0x64) & 0b1) {
            return;
        }
    }
}

void MouseWrite(uint8_t value) {
    MouseWait();
    outb(0x64, 0xD4);
    MouseWait();
    outb(0x60, value);
}

uint8_t MouseRead() {
    MouseWaitInput();
    return inb(0x60);
}

void handle_mouse(uint8_t data) {
    switch(MouseCycle) {
        case 0:
            if (MousePacketReady) break;
            if ((data & 0b00001000) == 0) break;
            MousePacket[0] = data;
            MouseCycle++;
        break;
        case 1:
            if (MousePacketReady) break;
            MousePacket[1] = data;
            MouseCycle++;
        break;
        case 2:
            if (MousePacketReady) break;
            MousePacket[2] = data;
            MousePacketReady = 1;
            MouseCycle = 0;
        break;
    }
}

uint8_t get_mouse(struct ps2_mouse_status* status) {
    if (status) {
        memcpy(status, &LastMouseStatus, sizeof(struct ps2_mouse_status));
        return 1;
    } else {
        return 0;
    }
}

uint8_t process_mouse_packet(struct ps2_mouse_status* status, uint8_t* buffer) {
    if (!buffer) return 0;
    if (!status) return 0;
    Point MousePosition = {LastMouseStatus.x, LastMouseStatus.y};
    uint8_t xNegative, yNegative, xOverflow, yOverflow;
    uint16_t screenWidth = screen.width;
    uint16_t screenHeight = screen.height;

    status->x = 0;
    status->y = 0;
    status->buttons = 0;
    status->scroll = 0;
    status->flags = 0;

    if (buffer[0] & PS2XSign) {
        xNegative = 1;
    } else {
        xNegative = 0;
    }

    if (buffer[0] & PS2YSign) {
        yNegative = 1;
    } else {
        yNegative = 0;
    }

    if (buffer[0] & PS2XOverflow) {
        xOverflow = 1;
        status->flags |= MOUSE_X_OVERFLOW;
    } else {
        xOverflow = 0;
    }

    if (buffer[0] & PS2YOverflow) {
        yOverflow = 1;
        status->flags |= MOUSE_Y_OVERFLOW;
    } else {
        yOverflow = 0;
    }    

    if (!xNegative) {
        MousePosition.x += buffer[1];
        if (xOverflow) {
            MousePosition.x  += 256;
        }
    } else {
        buffer[1] = 256 - buffer[1];
        MousePosition.x -= buffer[1];
        if (xOverflow) {
            MousePosition.x  -= 256;
        }
    }

    if (!yNegative) {
        MousePosition.y -= buffer[2];
        if (yOverflow) {
            MousePosition.y  -= 256;
        }
    } else {
        buffer[2] = 256 - buffer[2];
        MousePosition.y += buffer[2];
        if (yOverflow) {
            MousePosition.y  += 256;
        }
    }

    if (MousePosition.x < 0) {
        MousePosition.x = 0;
    }
    if (MousePosition.x > screenWidth-1) {
        MousePosition.x = screenWidth-1;
    }
    if (MousePosition.y < 0) {
        MousePosition.y = 0;
    }
    if (MousePosition.y > screenHeight-1) {
        MousePosition.y = screenHeight-1;
    }

    status->x = MousePosition.x;
    status->y = MousePosition.y;
    //Here: Clear mouse cursor and draw new one at coords
    //printf("\r(%d,%d)", MousePosition.x, MousePosition.y);
    
    if (buffer[0] & PS2LeftButton) {
        status->buttons |= MOUSE_LEFT_CLICK;
    } else {
        status->buttons &= ~MOUSE_LEFT_CLICK;
    }

    if (buffer[0] & PS2MiddleButton) {
        status->buttons |= MOUSE_MIDDLE_CLICK;
    } else {
        status->buttons &= ~MOUSE_MIDDLE_CLICK;
    }

    if (buffer[0] & PS2RightButton) {
        status->buttons |= MOUSE_RIGHT_CLICK;
    } else {
        status->buttons &= ~MOUSE_RIGHT_CLICK;
    }
    
    LastMouseStatus.x = status->x;
    LastMouseStatus.y = status->y;
    LastMouseStatus.buttons = status->buttons;
    LastMouseStatus.scroll = status->scroll;
    LastMouseStatus.flags = status->flags;
    return 1;
}

uint8_t process_current_mouse_packet(struct ps2_mouse_status* status) {
    if (MousePacketReady) {
        if (process_mouse_packet(status, MousePacket)) {
            MousePacketReady = 0;
            return 1;
        }
    } else {
        return 0;
    }
}

void init_mouse(uint16_t width, uint16_t height) {
    screen.width = width;
    screen.height = height;
    outb(0x64, 0xA8); // Enable auxiliary device - mouse

    MouseWait();

    outb(0x64, 0x20); //Tell keyboard ctrl to send data to mouse
    
    MouseWaitInput();

    uint8_t status = inb(0x60);
    
    MouseWait();
    outb(0x64, 0x60);
    MouseWait();
    outb(0x60, status); //set bit compaq status

    MouseWrite(0xF6); //Enable mouse default settings
    MouseRead();

    MouseWrite(0xF4);
    MouseRead();
}