static void *kmalloc(size_t size, gfp_t flags);
//allocates memory through slab allocator.

static inline void *kzalloc(size_t size, gfp_t flags);
//allocates memory (and zeroes it out like calloc() in libc) through the slab allocator.

void * krealloc(const void *, size_t, gfp_t);
//resize existing allocation.

void kfree(const void *);
//frees memory previously allocated.

void kzfree(const void *);