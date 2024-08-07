
#include <omen/libraries/basic/circular_list.h>

status_t circlist_init(circlist_t * circ, void * addr, size_t size) {
    if (circ == NULL || addr == NULL || size == 0) {
        return INVALID_ARGUMENT;
    }

    circ->addr = addr;
    circ->size = size;
    circ->head = 0;
    circ->tail = 0;

    return SUCCESS;
}

uint64_t circlist_free_space(circlist_t * circ) {
    if (circ == NULL) {
        return 0;
    }

    if (circ->head >= circ->tail) {
        return circ->size - (circ->head - circ->tail);
    } else {
        return circ->tail - circ->head;
    }
}

uint64_t circlist_used_space(circlist_t * circ) {
    if (circ == NULL) {
        return 0;
    }

    return circ->size - circlist_free_space(circ);
}

status_t circlist_write(circlist_t * circ, void * data, size_t size) {
    if (circ == NULL || data == NULL || size == 0) {
        return INVALID_ARGUMENT;
    }

    if (size > circ->size) {
        return INVALID_ARGUMENT;
    }

    if (circlist_free_space(circ) < size) {
        //make space
        while (circlist_free_space(circ) < size) {
            circ->tail = (circ->tail + 1) % circ->size;
        }
    }

    size_t i = 0;
    for (i = 0; i < size; i++) {
        ((uint8_t *)circ->addr)[circ->head] = ((uint8_t *)data)[i];
        circ->head = (circ->head + 1) % circ->size;
    }

    return SUCCESS;
}

status_t circlist_read(circlist_t * circ, void * data, size_t size) {
    if (circ == NULL || data == NULL || size == 0) {
        return INVALID_ARGUMENT;
    }

    if (size > circ->size) {
        return INVALID_ARGUMENT;
    }

    if (circlist_used_space(circ) < size) {
        return FAILURE;
    }

    size_t i = 0;
    for (i = 0; i < size; i++) {
        ((uint8_t *)data)[i] = ((uint8_t *)circ->addr)[circ->tail];
        circ->tail = (circ->tail + 1) % circ->size;
    }

    return SUCCESS;
}


bool circlist_is_empty(circlist_t * circ) {
    return circ->head == circ->tail;
}

bool circlist_is_full(circlist_t * circ) {
    return (circ->head + 1) % circ->size == circ->tail;
}