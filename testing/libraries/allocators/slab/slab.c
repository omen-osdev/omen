#include "slab.h"

#include "../modules/allocators.h"

kmem_cache_t *kmem_cache_create(char * name, size_t size, int align, void (*constructor)(void *, size_t), void (*destructor)(void *, size_t));
void kmem_cache_destroy(kmem_cache_t *cp);

void kmem_cache_reap(kmem_cache_t *cp);
void kmem_cache_shrink(kmem_cache_t *cp);

void *kmem_cache_alloc(kmem_cache_t *cp, int flags);
void kmem_cache_free(kmem_cache_t *cp, void *buf);

//kmalloc
void *kmalloc(size_t size, int flags);
void kfree(void *buf);