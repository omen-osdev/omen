#include "fifo.h"
#include <omen/managers/dev/fifo.h>

uint64_t fifo_dd_read(uint64_t port, uint64_t size, uint64_t skip, uint8_t* buffer) {
    struct fifo * fifo = (struct fifo*)port;
    uint64_t i = 0;
    while (i < size && fifo_size(fifo) > 0) {
        if (skip > 0) {
            fifo_get(fifo);
            skip--;
            continue;
        }
        buffer[i] = fifo_get(fifo);
        i++;
    }

    return i;
}

uint64_t fifo_dd_write(uint64_t port, uint64_t size, uint64_t skip, uint8_t* buffer) {
    (void)skip;
    struct fifo * fifo = (struct fifo*)port;
    uint64_t i = 0;
    while (i < size) {
        fifo_put(fifo, buffer[i]);
        i++;
    }

    return i;
}

uint64_t fifo_dd_ioctl(uint64_t port, uint32_t op, void* data) {
    (void)data;
    struct fifo * fifo = (struct fifo*)port;

    switch (op) {
        case IOCTL_FIFO_FLUSH:
            fifo_flush(fifo);
            return 1;
        case IOCTL_FIFO_GET_SIZE:
            return fifo_size(fifo);
        case IOCTL_FIFO_VALIDATE:
            return 1;
        default:
            return 0;
    }

    return 0;
}

struct file_operations fifo_fops = {
   .read = fifo_dd_read,
   .write = fifo_dd_write,
   .ioctl = fifo_dd_ioctl
};

void init_fifo_dd() {
    register_char(DEVICE_FIFO, FIFO_DD_NAME, &fifo_fops);
}