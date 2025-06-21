#include "tty_dd.h"
#include "tty.h"

#include <serial/serial_dd.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/std/termios.h>
#include <omen/apps/debug/debug.h>
#include <omen/managers/dev/devices.h>
#include <omen/libraries/allocators/heap_allocator.h>

uint64_t tty_dd_read_block_direct(struct tty* dev, uint64_t size, uint64_t skip, uint8_t* buffer) {
    
    if (skip) {
        char * skip_buffer = kmalloc(skip);
        if ((uint64_t)(int)skip != skip) {
            DBG_WARN("Skipping more than 2^32 bytes in _tty_read\n");
        }

        _tty_read(dev, skip_buffer, (int)skip);
        kfree(skip_buffer);
    }
    
    if ((uint64_t)(int)size != size) {
        DBG_WARN("Reading more than 2^32 bytes in _tty_read\n");
    }
    
    _tty_read(dev, (char*)buffer, (int)size);
    return strlen((char*)buffer);
}

uint64_t tty_dd_write_block_direct(struct tty* dev, uint64_t size, uint64_t skip, uint8_t* buffer) {
    buffer += skip;
    if ((uint64_t)(int)size != size) {
        DBG_WARN("Writing more than 2^32 bytes in _tty_write\n");
    }
    _tty_write(dev, (char*)buffer, (int)size);

    return size;
}

uint64_t tty_dd_read_block(uint64_t port, uint64_t size, uint64_t skip, uint8_t* buffer) {
    struct tty* device = get_tty((int)port);
    if (!is_valid_tty(device)) return 0;

    char * skip_buffer = kmalloc(skip);
    if ((uint64_t)(int)skip != skip) {
        DBG_WARN("Skipping more than 2^32 bytes in tty_dd_read_block\n");
    }
    _tty_read(device, skip_buffer, (int)skip);
    kfree(skip_buffer);
    if ((uint64_t)(int)size != size) {
        DBG_WARN("Reading more than 2^32 bytes in tty_dd_read_block\n");
    }
    _tty_read(device, (char*)buffer, (int)size);
    return strlen((char*)buffer);
}

uint64_t tty_dd_write_block(uint64_t port, uint64_t size, uint64_t skip, uint8_t* buffer) {
    //DBG_DEBUG("Writing to serial port %x (size: %d, skip: %d)\n", port, size, skip);
    struct tty* device = get_tty((int)port);
    if (!is_valid_tty(device)) {
        DBG_ERROR("Invalid TTY device\n");
        return 0;
    }

    buffer += skip;
    if ((uint64_t)(int)size != size) {
        DBG_WARN("Writing more than 2^32 bytes in tty_dd_write_block\n");
    }
    _tty_write(device, (char*)buffer, (int)size);

    return size;
}

uint64_t tty_dd_ioctl(uint64_t port, uint32_t op, void* data) {
    struct tty* device = get_tty((int)port);
    if (!is_valid_tty(device)) return 0;
    
    switch (op) {
        case TTY_ADD_SUBSCRIBER: {
            _tty_add_subscriber(device, (void (*)(void*, uint8_t))data);
            return 1;
        }
        case TTY_REMOVE_SUBSCRIBER: {
            _tty_remove_subscriber(device, (void (*)(void*, uint8_t))data);
            return 1;
        }
        case TTY_MODE_RAW: {
            _tty_modes(device, _TTY_MODE_RAW, *(int*)data);
            return 1;
        }
        case TTY_MODE_ECHO: {
            _tty_modes(device, _TTY_MODE_ECHO, *(int*)data);
            return 1;
        }
        case TTY_FLUSH: {
            _tty_flush(device);
            return 1;
        }
        case TTY_VALIDATE: {
            return is_valid_tty(device);
        }
        case TTY_GET_SIZE: {
            return _tty_get_size(device);
        }
        case TTY_HAS_INPUT: {
            return _tty_has_input(device);
        }
        case TTY_GWINSZ: {
            struct tty_winsize* ws = (struct tty_winsize*)data;
            ws->ws_col = device->cols;
            ws->ws_row = device->rows;
            return TTY_CHECK_VAL;
        }
        case TTY_GET_SERIAL_SETTINGS: {
            struct termios* termios = (struct termios*)data;
            struct termios* current_termios = _tty_get_termios(device);
            memcpy(termios, current_termios, sizeof(struct termios));
            return TTY_CHECK_VAL;
        }
        case TTY_SET_SERIAL_SETTINGS: {
            if (device->mode != TTY_MODE_SERIAL) {
                return 0;
            }
            struct termios* termios = (struct termios*)data;
            _tty_set_termios(device, termios);
            struct serial_ioctl_configuration iconfig,oconfig;
            iconfig.baud_rate = termios->ibaud;
            iconfig.parity = termios->c_cflag & PARENB;
            iconfig.stop_bits = termios->c_cflag & CSTOPB;
            iconfig.data_bits = termios->c_cflag & CSIZE;
            oconfig.baud_rate = termios->obaud;
            oconfig.parity = termios->c_cflag & PARENB;
            oconfig.stop_bits = termios->c_cflag & CSTOPB;
            oconfig.data_bits = termios->c_cflag & CSIZE;

            device_ioctl(device->indev, SERIAL_SET_CONFIG, (void*)&iconfig);
            device_ioctl(device->outdev, SERIAL_SET_CONFIG, (void*)&oconfig);
            return 1;
        }
        case TTY_SET_SERIAL_SETTINGS_WITHOUT_FLUSH: {
            if (device->mode != TTY_MODE_SERIAL) {
                return 0;
            }
            struct termios* termios = (struct termios*)data;
            _tty_set_termios(device, termios);
            struct serial_ioctl_configuration iconfig,oconfig;
            iconfig.baud_rate = termios->ibaud;
            iconfig.parity = termios->c_cflag & PARENB;
            iconfig.stop_bits = termios->c_cflag & CSTOPB;
            iconfig.data_bits = termios->c_cflag & CSIZE;
            oconfig.baud_rate = termios->obaud;
            oconfig.parity = termios->c_cflag & PARENB;
            oconfig.stop_bits = termios->c_cflag & CSTOPB;
            oconfig.data_bits = termios->c_cflag & CSIZE;

            device_ioctl(device->outdev, SERIAL_FLUSH_TX, 0);
            device_ioctl(device->indev, SERIAL_FLUSH_TX, 0);
            
            device_ioctl(device->indev, SERIAL_SET_CONFIG, (void*)&iconfig);
            device_ioctl(device->outdev, SERIAL_SET_CONFIG, (void*)&oconfig);
            return 1;
        }
        case TTY_SET_SERIAL_SETTINGS_WITH_FLUSH: {
                    if (device->mode != TTY_MODE_SERIAL) {
                return 0;
            }
            struct termios* termios = (struct termios*)data;
            _tty_set_termios(device, termios);
            struct serial_ioctl_configuration iconfig,oconfig;
            iconfig.baud_rate = termios->ibaud;
            iconfig.parity = termios->c_cflag & PARENB;
            iconfig.stop_bits = termios->c_cflag & CSTOPB;
            iconfig.data_bits = termios->c_cflag & CSIZE;
            oconfig.baud_rate = termios->obaud;
            oconfig.parity = termios->c_cflag & PARENB;
            oconfig.stop_bits = termios->c_cflag & CSTOPB;
            oconfig.data_bits = termios->c_cflag & CSIZE;

            device_ioctl(device->outdev, SERIAL_FLUSH_TX, 0);
            device_ioctl(device->indev, SERIAL_FLUSH_TX, 0);

            device_ioctl(device->outdev, SERIAL_DISCARD, 0);
            device_ioctl(device->indev, SERIAL_DISCARD, 0);

            device_ioctl(device->indev, SERIAL_SET_CONFIG, (void*)&iconfig);
            device_ioctl(device->outdev, SERIAL_SET_CONFIG, (void*)&oconfig);
            return 1;
        }
        default:
            return 0;
    }
    return 0;
}

struct file_operations tty_fops = {
   .read = tty_dd_read_block,
   .write = tty_dd_write_block,
   .ioctl = tty_dd_ioctl
};

void init_tty_dd() {
   register_block(DEVICE_TTY, TTY_DD_NAME, &tty_fops);
}