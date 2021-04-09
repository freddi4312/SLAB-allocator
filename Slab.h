#pragma once

struct MEMPART
{
  MEMPART *next;
  MEMPART *prev;
};

struct SLAB
{
  SLAB *next;
  SLAB *prev;
  size_t size;
  MEMPART *memPart;
  bool isSlabMarked;
};

struct cache
{
  SLAB *free;
  SLAB *partlyFree;
  SLAB *booked;
  size_t object_size;
  int slab_order;
  size_t slab_objects;
};

void cache_setup(cache *cache, size_t object_size);
void cache_release(cache *cache);
void *cache_alloc(cache *cache);
void cache_free(cache *cache, void *ptr);
void cache_shrink(cache *cache);