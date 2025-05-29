#include "tty_interface.h"
#include "tty_dd.h"
#include "tty.h"
#include <omen/managers/dev/devices.h>


char * create_tty(char* in, char* out, int mode, int inbs, int outbs) {
    int index = tty_init(in, out, mode, inbs, outbs);
    struct tty * tty = get_tty(index);
    if (!is_valid_tty(tty)) {
        return 0;
    }
    return device_create((void*)tty, DEVICE_TTY, index);
}

void destroy_tty(const char * device) {
    struct device* tty_dev = device_search(device);
    struct tty* tty = (struct tty*)tty_dev->internal_id;
    tty_destroy(tty);
    device_destroy(device);
}


uint64_t tty_read(const char * device, uint8_t * buffer, uint32_t skip, uint32_t size) {
    return device_read(device, size, skip, buffer);
}

uint64_t tty_write(const char * device, uint8_t * buffer, uint32_t skip, uint32_t size) {
    return device_write(device, size, skip, buffer);
}

uint64_t tty_write_now(const char * device, const char * buffer, uint32_t skip, uint32_t size) {
    uint64_t res = device_write(device, size, skip, (uint8_t*)buffer);
    device_ioctl(device, TTY_FLUSH, 0);
    return res;
}

uint64_t tty_read_direct(struct tty* tty, uint8_t * buffer, uint32_t skip, uint32_t size) {
    return tty_dd_read_block_direct(tty, size, skip, buffer);
}

uint64_t tty_write_direct(struct tty* tty, uint8_t * buffer, uint32_t skip, uint32_t size) {
    return tty_dd_write_block_direct(tty, size, skip, buffer);
}

uint64_t tty_ioctl(const char * device, uint32_t op, void * buffer) {
    return device_ioctl(device, op, buffer);
}

uint64_t tty_identify(const char* device) {
    return device_identify(device, TTY_DD_NAME);
}

uint64_t tty_add_subscriber(const char* device, void (*subscriber)(void*, uint8_t)) {
    return device_ioctl(device, TTY_ADD_SUBSCRIBER, subscriber);
}

uint64_t tty_remove_subscriber(const char* device, void (*subscriber)(void*, uint8_t)) {
    return device_ioctl(device, TTY_REMOVE_SUBSCRIBER, subscriber);
}

uint64_t tty_flush(const char* device) {
    return device_ioctl(device, TTY_FLUSH, 0);
}

uint64_t tty_set_echo(const char* device, uint8_t echo) {
    return device_ioctl(device, TTY_MODE_ECHO, (void*)&echo);
}

uint64_t tty_set_raw(const char* device, uint8_t raw) {
    return device_ioctl(device, TTY_MODE_RAW, (void*)&raw);
}

uint64_t tty_get_size(const char* device) {
    return device_ioctl(device, TTY_GET_SIZE, 0);
}

uint64_t tty_has_input(const char* device) {
    return device_ioctl(device, TTY_HAS_INPUT, 0);
}

uint64_t tty_validate(const char* device) {
    return device_ioctl(device, TTY_VALIDATE, 0);
}