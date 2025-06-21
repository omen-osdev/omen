#include "serial_dd.h"
#include "serial.h"
#include <omen/managers/dev/devices.h>
#include <omen/apps/debug/debug.h>

uint64_t serial_dd_read_block(uint64_t port, uint64_t size, uint64_t skip, uint8_t* buffer) {
    struct serial_device* device = get_serial((int)port);
    if (!device) return 0;

    for (uint64_t i = 0; i < skip; i++) {
        read_inb(device);
    }
    for (uint64_t i = 0; i < size; i++) {
        buffer[i] = read_inb(device);
        if (buffer[i] == 0) return i;
    }
    return size;
}

uint64_t serial_dd_write_block(uint64_t port, uint64_t size, uint64_t skip, uint8_t* buffer) {
    //DBG_DEBUG("Writing to serial port %x (size: %d, skip: %d)\n", port, size, skip);
    struct serial_device* device = get_serial((int)port);
    if (!device) return 0;

    buffer += skip;

    for (uint64_t i = 0; i < size; i++) {
        write_outb(device, buffer[i]);
    }

    return size;
}

uint64_t serial_dd_ioctl(uint64_t port, uint32_t op, void* data) {
    (void)data;
    
    switch (op) {
        case SERIAL_FLUSH_TX: {
            _serial_flush(port);
            return 1;
        }
        case SERIAL_SUBSCRIBE_READ: {
            struct serial_ioctl_subscriptor* subscriptor = (struct serial_ioctl_subscriptor*)data;
            serial_read_event_add(port, subscriptor->parent, subscriptor->handler);
            return 1;
        }
        case SERIAL_UNSUBSCRIBE_READ: {
            struct serial_ioctl_subscriptor* subscriptor = (struct serial_ioctl_subscriptor*)data;
            serial_read_event_remove(port, subscriptor->parent, subscriptor->handler);
            return 1;
        }
        case SERIAL_SUBSCRIBE_WRITE: {
            struct serial_ioctl_subscriptor* subscriptor = (struct serial_ioctl_subscriptor*)data;
            serial_write_event_add(port, subscriptor->parent, subscriptor->handler);
            return 1;
        }
        case SERIAL_UNSUBSCRIBE_WRITE: {
            struct serial_ioctl_subscriptor* subscriptor = (struct serial_ioctl_subscriptor*)data;
            serial_write_event_remove(port, subscriptor->parent, subscriptor->handler);
            return 1;
        }
        case SERIAL_ENABLE_ECHO: {
            serial_echo_enable(port);
            return 1;
        }
        case SERIAL_DISABLE_ECHO: {
            serial_echo_disable(port);
            return 1;
        }
        case SERIAL_DISCARD: {
            serial_discard(port);
            return 1;
        }
        case SERIAL_GET_CONFIG: {
            struct serial_ioctl_configuration* config = (struct serial_ioctl_configuration*)data;
            struct serial_device* device = get_serial((int)port);
            if (!device) return 0;
            config->baud_rate = device->config.baud_rate;
            config->parity = device->config.parity;
            config->stop_bits = device->config.stop_bits;
            config->data_bits = device->config.data_bits;
            return 1;
        }
        case SERIAL_SET_CONFIG: {
            struct serial_ioctl_configuration* config = (struct serial_ioctl_configuration*)data;
            struct serial_device* device = get_serial((int)port);
            if (!device) return 0;
            device->config.baud_rate = config->baud_rate;
            device->config.parity = config->parity;
            device->config.stop_bits = config->stop_bits;
            device->config.data_bits = config->data_bits;
            reconfigure_serial_comm(device->port, device->config.baud_rate, device->config.parity, device->config.stop_bits, device->config.data_bits);
            return 1;
        }
        default:
            return 0;
    }
    return 0;
}

struct file_operations serial_fops = {
   .read = serial_dd_read_block,
   .write = serial_dd_write_block,
   .ioctl = serial_dd_ioctl
};

void init_serial_dd() {
   register_char(DEVICE_SERIAL, SERIAL_DD_NAME, &serial_fops);
}