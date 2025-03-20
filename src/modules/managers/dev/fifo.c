#include <omen/managers/dev/fifo.h>
#include <omen/libraries/allocators/heap_allocator.h>

struct fifo* fifo_alloc(uint64_t size) {
    struct fifo *fifo = (struct fifo*)kmalloc(sizeof(struct fifo));
    if (!fifo) return (struct fifo*)0;
    fifo->buf = (uint8_t*)kmalloc(size);
    fifo->size = size;
    fifo->head = 0;
    fifo->tail = 0;
    return fifo;
}

void fifo_kfree(struct fifo *fifo) {
    kfree(fifo->buf);
    kfree(fifo);
}

uint8_t fifo_put(struct fifo *fifo, uint8_t data) {
    if (fifo->head == (fifo->tail + 1) % fifo->size) return 0;
    fifo->buf[fifo->tail] = data;
    fifo->tail = (fifo->tail + 1) % fifo->size;
    return 1;
}
uint8_t fifo_get(struct fifo *fifo) {
    if (fifo->head == fifo->tail) return 0;
    uint8_t data = fifo->buf[fifo->head];
    fifo->head = (fifo->head + 1) % fifo->size;
    return data;
}
uint64_t fifo_size(struct fifo *fifo) {
    return (fifo->tail - fifo->head + fifo->size) % fifo->size;
}

void fifo_flush(struct fifo *fifo) {
    fifo->head = 0;
    fifo->tail = 0;
}

const char* register_fifo(char* (*cb)(void*, uint8_t, uint64_t)) {
    struct fifo *fifo = fifo_alloc(1024);
    if (!fifo) return (const char*)0;
    return cb((void*)0, FIFO_MAJOR, (uint64_t)fifo);
}
