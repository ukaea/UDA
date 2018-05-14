#ifndef UDA_PLUGIN_EXP2IMAS_EXP2IMAS_RAMCACHE_H
#define UDA_PLUGIN_EXP2IMAS_EXP2IMAS_RAMCACHE_H

#include <stdlib.h>

typedef struct RamCache RAM_CACHE;

RAM_CACHE* ram_cache_new(size_t max_items);

void ram_cache_add(RAM_CACHE* cache, const char* key, void* value, size_t value_nbytes);

void* ram_cache_get(RAM_CACHE* cache, const char* key);

#endif // UDA_PLUGIN_EXP2IMAS_EXP2IMAS_RAMCACHE_H
