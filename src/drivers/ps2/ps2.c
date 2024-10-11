#include <omen/libraries/std/string.h>
#include <omen/libraries/std/stdint.h>
#include <omen/managers/dev/devices.h>
#include <omen/hal/arch/x86/io.h>
#include <omen/hal/arch/x86/cpu.h>
#include <omen/hal/arch/x86/int.h>
#include <omen/libraries/allocators/heap_allocator.h>

#include "ps2.h"
#include "mouse.h"
#include "keyboard.h"

struct ps2_subscriber {
    void * parent;
    void (*handler)(void* data, char c, int ignore);
    struct ps2_subscriber *next;
};

struct ps2_subscriber *keyboard_all_subscribers = 0;
struct ps2_subscriber *mouse_all_subscribers = 0;
struct ps2_subscriber *keyboard_event_subscribers = 0;
struct ps2_subscriber *mouse_event_subscribers = 0;

void KeyboardInt_Handler(cpu_context_t* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    uint8_t scancode = inb(0x60);
    char result = handle_keyboard(scancode);

    struct ps2_subscriber *current = keyboard_all_subscribers;
    while (current) {
        current->handler(current->parent, result, 0);
        current = current->next;
    }
    current = keyboard_event_subscribers;
    while (current) {
        current->handler(current->parent, result, 0);
        current = current->next;
    }
}

void MouseInt_Handler(cpu_context_t* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    struct ps2_mouse_status mouse;
    uint8_t scancode = inb(0x60);
    handle_mouse(scancode);

    if (process_current_mouse_packet(&mouse)) {
        struct ps2_subscriber *current = mouse_all_subscribers;
        while (current) {
            if (current->handler)
                current->handler((void*)&mouse, 0, 0);
            current = current->next;
        }
        if (mouse.buttons) {
            current = mouse_event_subscribers;
            while (current) {
                if (current->handler)
                    current->handler((void*)&mouse, 0, 0);
                current = current->next;
            }
        }
    }
}

uint8_t read_mouse(struct ps2_mouse_status * mouse) {
    if (get_mouse(mouse)) {
        return 1;
    } else {
        return 0;
    }
}

uint8_t read_keyboard(char * key) {
    *key = get_last_key();
    return 1;
}

void keyboard_halt_until_enter() {
    halt_until_enter();
}

void ps2_subscribe(void* handler, uint8_t device, uint8_t event) {
    struct ps2_subscriber *new_subscriber = (struct ps2_subscriber*)kmalloc(sizeof(struct ps2_subscriber));
    memset(new_subscriber, 0, sizeof(struct ps2_subscriber));
    struct ps2_subscriber *current = 0;
    if (device == PS2_DEVICE_MOUSE) {
        new_subscriber->parent = 0x0;
        new_subscriber->handler = handler;
        
        if (event == PS2_DEVICE_GENERIC_EVENT) {
            current = mouse_all_subscribers;
            new_subscriber->next = current;
            mouse_all_subscribers = new_subscriber;
        } else if (event == PS2_DEVICE_SPECIAL_EVENT) {
            current = mouse_event_subscribers;
            new_subscriber->next = current;
            mouse_event_subscribers = new_subscriber;
        } else {
            return;
        }
        
    } else if (device == PS2_DEVICE_KEYBOARD) {
        struct ps2_kbd_ioctl_subscriptor *kbdsub = (struct ps2_kbd_ioctl_subscriptor*)handler;
        new_subscriber->parent = kbdsub->parent;
        new_subscriber->handler = kbdsub->handler;

        if (event == PS2_DEVICE_GENERIC_EVENT) {
            current = keyboard_all_subscribers;
            new_subscriber->next = current;
            keyboard_all_subscribers = new_subscriber;
        } else {
            return;
        }
    }
}

void ps2_unsubscribe(void *handler) {
    struct ps2_subscriber *current = keyboard_all_subscribers;
    struct ps2_subscriber *prev = 0;
    while (current) {
        if (current->handler == handler) {
            if (prev) {
                prev->next = current->next;
            } else {
                keyboard_all_subscribers = current->next;
            }
            kfree(current);
            return;
        }
        prev = current;
        current = current->next;
    }

    current = mouse_all_subscribers;
    prev = 0;
    while (current) {
        if (current->handler == handler) {
            if (prev) {
                prev->next = current->next;
            } else {
                mouse_all_subscribers = current->next;
            }
            kfree(current);
            return;
        }
        prev = current;
        current = current->next;
    }

    current = keyboard_event_subscribers;
    prev = 0;
    while (current) {
        if (current->handler == handler) {
            if (prev) {
                prev->next = current->next;
            } else {
                keyboard_event_subscribers = current->next;
            }
            kfree(current);
            return;
        }
        prev = current;
        current = current->next;
    }

    current = mouse_event_subscribers;
    prev = 0;
    while (current) {
        if (current->handler == handler) {
            if (prev) {
                prev->next = current->next;
            } else {
                mouse_event_subscribers = current->next;
            }
            kfree(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

uint64_t mouse_dd_read(uint64_t port, uint64_t size, uint64_t skip, uint8_t* buffer) {
    (void)port;
    (void)size;
    (void)skip;
    struct ps2_mouse_status mouse;
    if (read_mouse(&mouse)) {
        memcpy(buffer, &mouse, sizeof(struct ps2_mouse_status));
        return 1;
    } else {
        return 0;
    }
}

uint64_t mouse_dd_write(uint64_t port, uint64_t size, uint64_t skip, uint8_t* buffer) {
    (void)port;
    (void)size;
    (void)skip;
    (void)buffer;
    return 0;
}

uint64_t mouse_dd_ioctl(uint64_t port, uint32_t op, void* data) {
    (void)port;
    switch (op) {
        case IOCTL_MOUSE_SUBSCRIBE: {
            ps2_subscribe(data, PS2_DEVICE_MOUSE, PS2_DEVICE_GENERIC_EVENT);
            return 1;
        }
        case IOCTL_MOUSE_UNSUBSCRIBE: {
            ps2_unsubscribe(data);
            return 1;
        }
        case IOCTL_MOUSE_SUBSCRIBE_EVENT: {
            ps2_subscribe(data, PS2_DEVICE_MOUSE, PS2_DEVICE_SPECIAL_EVENT);
            return 1;
        }
        case IOCTL_MOUSE_GET_STATUS: {
            struct ps2_mouse_status mouse;
            if (read_mouse(&mouse)) {
                memcpy(data, &mouse, sizeof(struct ps2_mouse_status));
                return 1;
            } else {
                return 0;
            }
        }
        default: {
            return 0;
        }
    }

    return 0;
}

uint64_t keyboard_dd_read(uint64_t port, uint64_t size, uint64_t skip, uint8_t* buffer) {
    (void)port;
    (void)size;
    (void)skip;
    char key;
    if (read_keyboard(&key)) {
        buffer[0] = key;
        return 1;
    } else {
        return 0;
    }
}

uint64_t keyboard_dd_write(uint64_t port, uint64_t size, uint64_t skip, uint8_t* buffer) {
    (void)port;
    (void)size;
    (void)skip;
    (void)buffer;
    return 0;
}

uint64_t keyboard_dd_ioctl(uint64_t port, uint32_t op, void* data) {
    (void)port;
    switch (op) {
        case IOCTL_KEYBOARD_SUBSCRIBE: {
            ps2_subscribe(data, PS2_DEVICE_KEYBOARD, PS2_DEVICE_GENERIC_EVENT);
            return 1;
        }
        case IOCTL_KEYBOARD_UNSUBSCRIBE: {
            ps2_unsubscribe(data);
            return 1;
        }
        case IOCTL_KEYBOARD_HALT_UNTIL_ENTER: {
            keyboard_halt_until_enter();
            return 1;
        }
        default: {
            return 0;
        }
    }

    return 0;
}

struct file_operations mouse_fops = {
   .read = mouse_dd_read,
   .write = mouse_dd_write,
   .ioctl = mouse_dd_ioctl
};

struct file_operations keyboard_fops = {
   .read = keyboard_dd_read,
   .write = keyboard_dd_write,
   .ioctl = keyboard_dd_ioctl
};

void init_ps2_dd(size_t width, size_t height) {
    init_keyboard();
    init_mouse(width, height);

    hook_interrupt(KBD_IRQ, (void*)KeyboardInt_Handler);
    hook_interrupt(MSE_IRQ, (void*)MouseInt_Handler);
    unmask_interrupt(KBD_IRQ);
    unmask_interrupt(MSE_IRQ);

    register_block(DEVICE_MOUSE, MOUSE_DD_NAME, &mouse_fops);
    register_char(DEVICE_KEYBOARD, KEYBOARD_DD_NAME, &keyboard_fops);
    device_create(0, DEVICE_MOUSE, 0);
    device_create(0, DEVICE_KEYBOARD, 0);
}