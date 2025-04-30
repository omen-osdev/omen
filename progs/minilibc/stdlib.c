#include <stdlib.h>
#include <minilibc.h>

//void * sys_mmap(void * addr, size_t length, int prot, int flags, int fd, off_t offset);
//void sys_mprotect(void * addr, size_t length, int prot);
//void sys_munmap(void * addr, size_t length);

void *malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    void *ptr = sys_mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        return NULL;
    }
    return ptr;
}

void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    sys_munmap(ptr, 0);
}