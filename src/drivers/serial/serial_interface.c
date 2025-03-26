#include "serial_interface.h"
#include "serial_dd.h"
#include <omen/managers/dev/devices.h>

uint64_t serial_read(const char * device, uint8_t * buffer, uint32_t skip, uint32_t size) {
    return device_read(device, size, skip, buffer);
}

//TODO: Doesnt really work
uint64_t serial_read_echo(const char * device, uint8_t * buffer, uint32_t skip, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = device_read(device, 1, skip, buffer);
        if (buffer[i] == 0) return i;
        serial_write_now(device, (char*)&buffer[i], 0, 1);
    }
    return size;
}

uint64_t serial_write(const char * device, uint8_t * buffer, uint32_t skip, uint32_t size) {
    return device_write(device, size, skip, buffer);
}
uint64_t serial_write_now(const char * device, const char * buffer, uint32_t skip, uint32_t size) {
    uint64_t res = device_write(device, size, skip, (uint8_t*)buffer);
    device_ioctl(device, SERIAL_FLUSH_TX, 0);
    return res;
}
uint64_t serial_ioctl(const char * device, uint32_t op, void * buffer) {
    return device_ioctl(device, op, buffer);
}
uint64_t serial_identify(const char * device) {
    return device_identify(device, SERIAL_DD_NAME);
}