#ifndef LC_POOL_H
#define LC_POOL_H

#include "common.h"

typedef uint32_t    pool_size_t;
typedef uint32_t    pool_obj_size_t;

typedef struct pool {
    pool_obj_size_t     objsize;
    pool_obj_size_t     objmemsize;

    pool_size_t         num;
    pool_size_t         free_idx;
    pool_size_t         *freeobj;   // pointer the first free obj

    void *obj;
    char buffer[0];
} pool_t;


typedef struct pool_obj_head {
    pool_t  *head;
} pool_obj_head_t;

/* APIs declare here*/

pool_t *pool_create(pool_obj_size_t objsize, pool_size_t poolsize);
void pool_destory(pool_t *p);

void *pool_alloc_obj(pool_t *p);
void *pool_free_obj(pool_t *p, void *obj);


#endif
