#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "common.h"
#include "list.h"


typedef void *htable_key_t;

typedef int(*htable_cmp_t) (htable_key_t, list_head_t *);
typedef uint32_t (*htable_hash_t)(htable_key_t);

typedef struct htable {
    int             size;
    htable_cmp_t    cmp;
    htable_hash_t   hash;
    list_head_t     bucket[0];
} htable_t;


uint32_t htable_defaut_hash(htable_key_t key, size_t len);
htable_t htable_create(int size, htable_cmp_t cmp, htable_hash_t hash);
void htable_destory(htable_t *ht);

list_head_t *htable_find(htable_t *ht, htable_key_t key);
void htable_delete(htable_t *ht, htable_key_t key);


#define for_each_htable_entry(t, pos, member)       \
    for(int b = t->size - 1; b >= 0; b--)           \
list_for_each_entry(pos, &t->bucket[b],member)

#endif
