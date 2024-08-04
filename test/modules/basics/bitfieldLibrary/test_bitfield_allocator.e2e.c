#include "test_bitfield_allocator.e2e.h"

void setUp(){}
void tearDown(){}

void *_allocate_stub(void *data) {
    struct _allocation_data *allocation = (struct _allocation_data *)data;
    struct bitfield *bf = allocation->bf;
    uint64_t size = allocation->size;

    return allocate(bf, size);
}

void *_deallocate_stub(void *data) {
    struct _allocation_data *allocation = (struct _allocation_data *)data;
    struct bitfield *bf = allocation->bf;
    void *address = allocation->address;
    uint64_t size = allocation->size;

    deallocate(bf, address, size);
    return NULL;
}

void *_check_stub(void *data) {
    struct _allocation_data *allocation = (struct _allocation_data *)data;
    struct bitfield *bf = allocation->bf;

    debug_bitfield(bf);

    return NULL;
}

void _add_allocation(struct _allocation_metadata *allocation, void *address, uint64_t size) {
    struct _allocation_tracker *tracker = (struct _allocation_tracker *)malloc(sizeof(struct _allocation_tracker));
    tracker->address = address;
    tracker->size = size;
    tracker->expected_fragmentation = size % allocation->page_size;
    tracker->next = NULL;

    pthread_mutex_lock(&allocation->tracker_lock);
    if (allocation->tracker == NULL) {
        allocation->tracker = tracker;
    } else {
        struct _allocation_tracker *current = allocation->tracker;
        while (current->next != NULL) {
            current = current->next;
        }
        allocation->allocated++;
        current->next = tracker;
    }
    pthread_mutex_unlock(&allocation->tracker_lock);
}

void _mark_as_freed(struct _allocation_metadata *allocation, void *address) {
    pthread_mutex_lock(&allocation->tracker_lock);
    struct _allocation_tracker *current = allocation->tracker;
    while (current != NULL) {
        if (current->address == address) {
            current->freed = 1;
            break;
        }
        current = current->next;
        allocation->allocated--;
    }
    pthread_mutex_unlock(&allocation->tracker_lock);
}

void _worker_thread(void *arg) {
    struct _allocation_metadata *data = (struct _allocation_metadata *)arg;
    uint64_t iterations = data->iterations;

    struct _allocation_data *allocation_data = (struct _allocation_data *)malloc(sizeof(struct _allocation_data));
    allocation_data->bf = (struct bitfield *)data->allocation_control_structure;

    for (uint64_t i = 0; i < iterations; i++) {
        printf("Thread %lu iteration %lu\n", pthread_self(), i);
        allocation_data->size = rand() % 0x10000;

        uint8_t operation = rand() % 2;
        if (operation == 0) { // Allocate
            void *address = _allocate_stub(allocation_data);
            if (address != NULL) {
                _add_allocation(data, address, allocation_data->size);
            }
        } else {
            if (data->allocated <= 0) {
                continue;
            }
            uint64_t chunk_no = rand() % data->allocated;
            struct _allocation_tracker *current = data->tracker;
            for (uint64_t j = 0; j < chunk_no; j++) {
                if (current == 0) {
                    break;
                }
                current = current->next;
            }
            if (current != NULL && !current->freed) {
                _mark_as_freed(data, current->address);
                allocation_data->address = current->address;
                _deallocate_stub(allocation_data);
            }
        }
    }
}

int main() {
    srand(time(NULL));

    uint64_t data_size = 0x100000000;
    uint16_t page_size = 0x1000;

    void *data = malloc(data_size);
    struct bitfield *bf = init(data, data_size, page_size);

    uint64_t thread_count = 5;
    uint64_t iterations = 0x100;

    struct _allocation_metadata *metadata = (struct _allocation_metadata *)malloc(sizeof(struct _allocation_metadata));
    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);

    metadata->allocation_control_structure = bf;
    metadata->iterations = iterations;
    metadata->tracker = NULL;
    metadata->allocated = 0;
    metadata->page_size = page_size;
    pthread_mutex_init(&metadata->tracker_lock, NULL);

    for (uint64_t i = 0; i < thread_count; i++) {
        pthread_create(&threads[i], NULL, (void *)_worker_thread, metadata);
    }

    for (uint64_t i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    uint64_t total_allocated = 0;
    uint64_t freed = 0;
    uint64_t still_allocated = 0;
    uint64_t fragmentation = 0;
    uint64_t expected_result = 0;

    struct _allocation_tracker *current = metadata->tracker;
    while (current != NULL) {
        total_allocated += current->size;
        if (current->freed) {
            freed += current->size;
        } else {
            still_allocated += current->size;
            fragmentation += current->expected_fragmentation;
            expected_result += current->size + current->expected_fragmentation;
        }
        current = current->next;
    }

    printf("Total allocated: %lu\n", total_allocated);
    printf("Freed: %lu\n", freed);
    printf("Still allocated: %lu\n", still_allocated);
    printf("Fragmentation: %lu\n", fragmentation);
    printf("Expected result: %lu\n", expected_result);

    _check_stub((void *)bf);

    return 0;
}
