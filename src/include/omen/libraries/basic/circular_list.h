#ifndef _CIRCLIST_H
#define _CIRCLIST_H

#include <omen/libraries/std/stdbool.h>
#include <omen/libraries/std/stddef.h>

typedef struct circlist {
    void * addr;
    size_t size;
    size_t head;
    size_t tail;
} circlist_t;

status_t circlist_init(circlist_t * circ, void * addr, size_t size);
status_t circlist_write(circlist_t * circ, void * data, size_t size);
status_t circlist_read(circlist_t * circ, void * data, size_t size);

bool circlist_is_empty(circlist_t * circ);
bool circlist_is_full(circlist_t * circ);

#endif