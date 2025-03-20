#ifndef _FIFO_H
#define _FIFO_H

#define FIFO_MAJOR 0x8e

#include <omen/libraries/std/stdint.h>

struct fifo {
    uint8_t *buf;
    uint64_t head;
    uint64_t tail;
    uint64_t size;
    uint8_t free;
};


struct fifo *fifo_alloc(uint64_t size);
void fifo_kfree(struct fifo *fifo);
uint8_t fifo_put(struct fifo *fifo, uint8_t data);
uint8_t fifo_get(struct fifo *fifo);
uint64_t fifo_size(struct fifo *fifo);
void fifo_flush(struct fifo *fifo);
const char* register_fifo(char* (*cb)(void*, uint8_t, uint64_t));
#endif