#include <omen/managers/cpu/sline.h>
#include <omen/apps/panic/panic.h>
#include <omen/managers/cpu/process.h>
#include <omen/managers/dev/pit.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/apps/debug/debug.h>
#include <omen/libraries/std/string.h>

struct snode {
    thread_t * thread;
    struct timespec * duration;
    struct timespec * rem;
    struct snode * next;
};

struct squeue {
    struct snode * head;
    int id;
    struct squeue * next;
};

struct squeue * squeue_head = 0x0;

void fuckup_detector(struct squeue * deleted_queue) {
    struct squeue * current = squeue_head;
    while (current != NULL) {
        if (current == deleted_queue) {
            panic("Fuckup detected\n");
        }
        current = current->next;
    }
}

struct squeue * create_squeue(int id) {
    // Create a new squeue and add it to the list
    struct squeue * new_squeue = (struct squeue *)kmalloc(sizeof(struct squeue));
    if (new_squeue == NULL) {
        panic("Failed to create squeue\n");
    }
    memset(new_squeue, 0, sizeof(struct squeue));
    new_squeue->head = NULL;
    new_squeue->id = id;
    new_squeue->next = NULL;
    if (squeue_head == NULL) {
        squeue_head = new_squeue;
    } else {
        struct squeue * current = squeue_head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_squeue;
    }
    return new_squeue;
}

struct snode * create_snode(thread_t * thread, const struct timespec *duration, struct timespec *rem) {
    // Create a new snode and add it to the squeue
    struct snode * new_snode = (struct snode *)kmalloc(sizeof(struct snode));
    if (new_snode == NULL) {
        panic("Failed to create snode\n");
    }
    memset(new_snode, 0, sizeof(struct snode));
    new_snode->thread = thread;
    new_snode->duration = NULL;
    if (duration != NULL) {
        new_snode->duration = (struct timespec *)kmalloc(sizeof(struct timespec));
        if (new_snode->duration == NULL) {
            panic("Failed to allocate memory for duration\n");
        }
        memset(new_snode->duration, 0, sizeof(struct timespec));
        memcpy(new_snode->duration, duration, sizeof(struct timespec));
    }
    new_snode->rem = rem;
    new_snode->next = NULL;
    return new_snode;
}

void destroy_snode(struct snode * node) {
    if (node == NULL) panic("Node is NULL\n");
    if (node->duration != NULL) {
        kfree(node->duration);
    }
    kfree(node);
}

void destroy_squeue(struct squeue * queue) {
    //Remove the queue from the queue list, then deallocate it
    if (queue == NULL) panic("Queue is NULL\n");
    struct squeue * current = squeue_head;
    struct squeue * prev = NULL;
    while (current != NULL) {
        if (current == queue) {
            if (prev == NULL) {
                squeue_head = current->next;
            } else {
                prev->next = current->next;
            }
            struct snode * node = current->head;
            while (node != NULL) {
                struct snode * temp = node;
                node = node->next;
                destroy_snode(temp);
            }
            kfree(current);
            fuckup_detector(current);
            return;
        }
        prev = current;
        current = current->next;
    }
    panic("Queue not found\n");
}

struct squeue * find_squeue(int id) {
    struct squeue * current = squeue_head;
    while (current != NULL) {
        if (current->id == id) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

struct squeue * find_squeue_by_thread(thread_t * thread) {
    struct squeue * current = squeue_head;
    while (current != NULL) {
        struct snode * node = current->head;
        while (node != NULL) {
            if (node->thread == thread) {
                return current;
            }
            node = node->next;
        }
        current = current->next;
    }
    return NULL;
}

struct snode * find_snode(struct squeue * queue, thread_t * thread) {
    struct snode * current = queue->head;
    while (current != NULL) {
        if (current->thread == thread) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void __sleep(thread_t * thread, int condition, struct timespec * rem, struct timespec * duration) {
    // Check if the thread is already sleeping
    thread->status = THREAD_STATUS_INTERRUPTIBLE_SLEEP;
    // Add the thread to the squeue
    struct squeue * queue = find_squeue(condition);
    if (queue == NULL) {
        queue = create_squeue(condition);
        if (queue == NULL) {
            panic("Failed to create squeue\n");
        }
    }
    struct snode * node = find_snode(queue, thread);
    if (node == NULL) {
        node = create_snode(thread, NULL, NULL);
        if (node == NULL) {
            panic("Failed to create snode\n");
        }
        node->next = queue->head;
        queue->head = node;
    }
}

void wakeup(int condition) {
    struct squeue * queue = find_squeue(condition);
    if (queue == NULL) {
        panic("No squeue found\n");
    }
    struct snode * current = queue->head;
    while (current != NULL) {
        current->thread->status = THREAD_STATUS_READY;
        current = current->next;
    }
    destroy_squeue(queue);
}

void __wakeup_alarm(int condition, uint64_t ticks, int64_t rearm_ticks) {
    wakeup(condition);
    remove_alarm(condition);
}

struct timespec * is_sleeping(thread_t * thread) {
    struct squeue * queue = find_squeue_by_thread(thread);
    if (queue == NULL) {
        return NULL;
    }
    struct snode * node = find_snode(queue, thread);
    if (node == NULL) {
        return NULL;
    }
    return node->rem;
}

void sleep_current(int condition) {
    thread_t * current_thread = get_current_thread();
    if (current_thread == NULL) {
        panic("Current thread is NULL\n");
    }
    struct timespec * rem = is_sleeping(current_thread);
    if (rem != NULL) {
        kprintf("Thread %d is already sleeping\n", current_thread->id);
        return;
    }
    __sleep(current_thread, condition, NULL, NULL);
}

void sleep(thread_t * thread, int condition) {
    __sleep(thread, condition, NULL, NULL);
}

void force_wakeup_task(thread_t * thread) {
    struct squeue * queue = find_squeue_by_thread(thread);
    if (queue == NULL) {
        kprintf("No threads to wake up\n");
        return;
    }
    int64_t remaining_ticks = get_remaining_ticks(queue->id);
    uint64_t remaining_ns = ticks_to_ns(remaining_ticks);
    if (remaining_ns == 0) {
        kprintf("No remaining ticks\n");
        return;
    }

    struct snode * node = find_snode(queue, thread);
    if (node == NULL) {
        panic("No snode found\n");
    }

    if (node->rem != NULL) {
        node->rem->tv_sec = remaining_ns / 1000000000;
        node->rem->tv_nsec = remaining_ns % 1000000000;
    }
    thread->status = THREAD_STATUS_READY;
    destroy_snode(node);
    if (queue->head == NULL) {
        remove_alarm(queue->id);
        destroy_squeue(queue);
    }
}

int nanosleep(thread_t * thread, struct timespec *duration, struct timespec *rem) {
    uint64_t ticks = ns_to_ticks(duration->tv_sec * 1000000000 + duration->tv_nsec);
    if (ticks == 0) {
        return -1;
    }

    int condition = add_alarm(ticks, 0, __wakeup_alarm);
    if (condition < 0) {
        panic("Failed to add alarm\n");
    }

    __sleep(thread, condition, rem, (struct timespec *)duration);
}