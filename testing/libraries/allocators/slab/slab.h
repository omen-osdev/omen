#ifndef _SLAB_H
#define _SLAB_H

#include <omen/modules/libraries/definitions/stddef.h> 
#include <omen/modules/libraries/definitions/stdint.h> //TODO: Delete this
#include "list.h" //TODO: Delete this

#define MAX_NUMNODES 16 //TODO: Move this to a better place (NUMA)

#define KM_SLEEP 0x1
#define KM_NOSLEEP 0x2

//TODO: Move this to a better place
struct reciprocal_value {
    uint32_t m;
    uint8_t sh1, sh2;
};
typedef unsigned int gfp_t;
struct list_head {
    struct list_head *next, *prev;
};

typedef int slab_flags_t;

struct kmem_cache_order_objects {
    unsigned int x;
};

typedef struct kmem_cache {
    struct kmem_cache_cpu *cpu_slab; //TODO: Implement __percpu
    slab_flags_t flags;
    unsigned int min_partial;
    unsigned int size;
    unsigned int object_size;
    struct reciprocal_value reciprocal_size;
    unsigned int offset;
    unsigned int cpu_partial;
    unsigned int cpu_partial_slabs;
    struct kmem_cache_order_objects oo;
    struct kmem_cache_order_objects min;
    gfp_t allocflags;
    int refcount;
    void (*ctor)(void *);
    unsigned int inuse;
    unsigned int align;
    unsigned int red_left_pad;
    const char * name;
    struct list_head list;
    unsigned int useroffset;
    unsigned int usersize;
    struct kmem_cache_node *node[MAX_NUMNODES];
} kmem_cache_t;
typedef struct kmem_cache_cpu {} kmem_cache_cpu_t;
typedef struct kmem_cache_node {} kmem_cache_node_t;
typedef struct slab {} slab_t;

kmem_cache_t *kmem_cache_create(char * name, size_t size, int align, void (*constructor)(void *, size_t), void (*destructor)(void *, size_t));
void kmem_cache_destroy(kmem_cache_t *cp);

void kmem_cache_reap(kmem_cache_t *cp);
void kmem_cache_shrink(kmem_cache_t *cp);

void *kmem_cache_alloc(kmem_cache_t *cp, int flags);
void kmem_cache_free(kmem_cache_t *cp, void *buf);

//kmalloc
void *kmalloc(size_t size, int flags);
void kfree(void *buf);
#endif