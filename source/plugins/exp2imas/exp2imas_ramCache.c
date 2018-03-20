#include "exp2imas_ramCache.h"

#include <clientserver/stringUtils.h>

typedef struct RamCache {
    size_t max_items;
    size_t current_idx;
    char** keys;
    void** values;
} RAM_CACHE;

RAM_CACHE* ram_cache_new(size_t max_items)
{
    RAM_CACHE* cache = malloc(sizeof(RAM_CACHE));

    cache->keys = (char**)calloc(max_items, sizeof(char*));
    cache->values = (void**)calloc(max_items, sizeof(void*));
    cache->max_items = max_items;
    cache->current_idx = 0;

    return cache;
}

void ram_cache_add(RAM_CACHE* cache, const char* key, void* value, size_t value_nbytes)
{
    free(cache->keys[cache->current_idx]);
    free(cache->values[cache->current_idx]);

    cache->keys[cache->current_idx] = strdup(key);
    cache->values[cache->current_idx] = malloc(value_nbytes);
    memcpy(cache->values[cache->current_idx], value, value_nbytes * sizeof(char));

    cache->current_idx = (cache->current_idx + 1) % cache->max_items;
}

void* ram_cache_get(RAM_CACHE* cache, const char* key)
{
    size_t i;
    for (i = 0; i < cache->max_items; ++i) {
        if (StringEquals(cache->keys[i], key)) {
            return cache->values[i];
        }
    }
    return NULL;
}
