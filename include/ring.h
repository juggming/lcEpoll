#ifndef LC_RING_H
#define LC_RING_H

#include "common.h"
#include <string.h>

/* lock-free single producer and single consumer queue */

typedef struct ring {
    unsigned int        size;
    unsigned int        mask;
    volatile unsigned int   head;
    volatile unsigned int   tail;
    void *volatile ring[0];
} ring_t;


static inline ring_t* ring_create(unsigned int count)
{
    ring_t    *r;
    size_t    ring_size;

    if(!POWEROF2(count))
        return NULL;

    ring_size = count * sizeof(void *) + sizeof(struct ring);

    r = malloc(ring_size);
    if(!r)
        return NULL;
    memset(r, 0, ring_size);
    r->size = count;
    r->mask = count - 1;

    return r;
}

static inline void ring_destroy(ring_t *r)
{
    free(r);
}


static inline unsigned int ring_len(ring_t *r)
{
    if(r->tail <= r->head)
        return (r->head - r->tail);
    else
        return (r->size - r->tail + r->head);
}

static inline bool ring_full(ring_t *r)
{
    return ((r->head + 1) & r->mask) == r->tail;
}

static inline bool ring_empty(ring_t *r)
{
    return (r->head == r->tail);
}

static inline void *ring_peek(ring_t *r)
{
    if(unlikely(r->head == r->tail))
        return NULL;

    return r->ring[r->tail];
}

static inline bool ring_enqueue(ring_t *r, void *data)
{
    if(unlikely(((r->head + 1) & r->mask) == r->tail))
        return false;

    r->ring[r->head] = data;
    // stolen from kernel.
    smp_wmb();
    r->head = (r->head + 1) & r->mask;

    return true;
}

#endif
