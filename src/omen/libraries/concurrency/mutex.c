#include "omen/libraries/concurrency/mutex.h"
#include <stdatomic.h>

void mutex_init(mutex_t *mutex) { atomic_flag_clear(&mutex->lock); }

void mutex_lock(mutex_t *mutex) {
    while (atomic_flag_test_and_set(&mutex->lock)) {
    }
}

void mutex_unlock(mutex_t *mutex) { atomic_flag_clear(&mutex->lock); }

void semaphore_init(semaphore_t *semaphore, int count) {
    semaphore->count = count;
    mutex_init(&semaphore->mutex);
}

void semaphore_wait(semaphore_t *semaphore) {
    mutex_lock(&semaphore->mutex);
    while (semaphore->count <= 0) {
        mutex_unlock(&semaphore->mutex);
        mutex_lock(&semaphore->mutex);
    }
    semaphore->count--;
    mutex_unlock(&semaphore->mutex);
}

void semaphore_signal(semaphore_t *semaphore) {
    mutex_lock(&semaphore->mutex);
    semaphore->count++;
    mutex_unlock(&semaphore->mutex);
}

void condition_init(condition_t *condition) {
    semaphore_init(&condition->mutex, 1);
    semaphore_init(&condition->wait, 0);
    condition->wait_count = 0;
}

void condition_wait(condition_t *condition) {
    semaphore_wait(&condition->mutex);
    condition->wait_count++;
    semaphore_signal(&condition->mutex);
    semaphore_wait(&condition->wait);
    condition->wait_count--;
}

void condition_signal(condition_t *condition) {
    semaphore_wait(&condition->mutex);
    if (condition->wait_count > 0) {
        semaphore_signal(&condition->wait);
    }
    semaphore_signal(&condition->mutex);
}

void condition_broadcast(condition_t *condition) {
    semaphore_wait(&condition->mutex);
    for (int i = 0; i < condition->wait_count; i++) {
        semaphore_signal(&condition->wait);
    }
    condition->wait_count = 0;
    semaphore_signal(&condition->mutex);
}

void barrier_init(barrier_t *barrier, int count) {
    barrier->count = count;
    barrier->total = 0;
    condition_init(&barrier->condition);
}

void barrier_wait(barrier_t *barrier) {
    semaphore_wait(&barrier->condition.mutex);
    barrier->total++;
    if (barrier->total == barrier->count) {
        barrier->total = 0;
        condition_broadcast(&barrier->condition);
    } else {
        condition_wait(&barrier->condition);
    }
    semaphore_signal(&barrier->condition.mutex);
}

void spinlock_init(spinlock_t *spinlock) { spinlock->lock = 0; }

void spinlock_lock(spinlock_t *spinlock) {
    while (atomic_exchange(&spinlock->lock, 1)) {
    }
}

void spinlock_unlock(spinlock_t *spinlock) { atomic_store(&spinlock->lock, 0); }
