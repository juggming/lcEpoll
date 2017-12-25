#include "hash_table.h"

#include <assert.h>

htable_t *htable_create(int size, htable_cmp_t cmp, htable_hash_t hash)
{
    int i;
    htable_t *t;

    assert(cmp);
    assert(hash);

    t = calloc(1, sizeof(htable_t) + sizeof(list_head_t) * size);
    if(!t)
        return NULL;

    t->size = size;
    t->cmp = cmp;
    t->hash = hash;

    for(i = 0; i < size; i++)
        INIT_LIST_HEAD(&t->bucket[i]);

    return t;
}

void htable_destroy(htable_t *t)
{
    free(t);
}

list_head_t *htable_find(htable_t *t, htable_key_t key)
{
    int pos = t->hash(key) % t->size;

    list_head_t *node;

    list_for_each(node, &t->bucket[pos]) {
        if(t->cmp(key, node))
            return node;
    }

    return NULL;
}

void htable_add(htable_t *t, htable_key_t key, list_head_t *data)
{
    int pos;

    list_head_t *node = htable_find(t, key);
    if(node)
        return;

    pos = t->hash(key) % t->size;
    list_add_tail(data, &t->bucket[pos]);
}

void htable_del(htable_t *t, htable_key_t key)
{
    list_head_t *node = htable_find(t, key);
    if(node)
        list_del(node);
}

/*
 * use the murmurhash2 from Google Code
 */

uint32_t htable_default_hash(htable_key_t key, size_t len)
{
    uint32_t h, k;
    uint32_t *data = key;

    h = 0^len;

    while(len >= 4) {
        k = data[0];
        k |= data[1] << 8;
        k |= data[2] << 16;
        k |= data[3] << 24;

        k *= 0x5bd1e995;
        k ^= k >> 24;
        k *= 0x5bd1e995;

        h *= 0x5bd1e995;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch(len) {
        case 3:
            h ^= data[2] << 16;
        case 2:
            h ^= data[1] << 8;
        case 1:
            h ^= data[0];
            h *= 0x5bd1e995;
    }

    h ^= h >> 13;
    h *= 0x5bd1e995;
    h ^= h >> 15;

    return h;
}
