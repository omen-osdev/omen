#ifndef MUTEX_H
#define MUTEX_H
#include <stdatomic.h>

typedef struct {
    atomic_flag lock;
} mutex_t;

typedef struct {
    atomic_int count;
    mutex_t mutex;
} semaphore_t;

typedef struct {
    semaphore_t mutex;
    semaphore_t wait;
    int wait_count;
} condition_t;

typedef struct {
    int count;
    int total;
    condition_t condition;
} barrier_t;

typedef struct {
    atomic_int lock;
} spinlock_t;

void mutex_init(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

void semaphore_init(semaphore_t *semaphore, int count);
void semaphore_wait(semaphore_t *semaphore);
void semaphore_signal(semaphore_t *semaphore);

void condition_init(condition_t *condition);
void condition_wait(condition_t *condition);
void condition_signal(condition_t *condition);
void condition_broadcast(condition_t *condition);

void barrier_init(barrier_t *barrier, int count);
void barrier_wait(barrier_t *barrier);

void spinlock_init(spinlock_t *spinlock);
void spinlock_lock(spinlock_t *spinlock);
void spinlock_unlock(spinlock_t *spinlock);

#endif // MUTEX_H