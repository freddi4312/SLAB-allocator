#include <algorithm>
#include "Slab.h"
#include "BuddyAlloc.h"



void cache_setup(cache *cache, size_t object_size)
{
  cache->free = NULL;
  cache->partlyFree = NULL;
  cache->booked = NULL;
  cache->object_size = std::max(sizeof(MEMPART), object_size);
  cache->slab_order = 10;
  cache->slab_objects = (((size_t)1 << (12 + cache->slab_order)) - sizeof(SLAB)) / cache->object_size;
}

void FreeSlabsFromList(SLAB *slab)
{
  while (slab != NULL)
  {
    SLAB *tmp = slab;
    slab = slab->next;
    free_slab((void *)tmp);
  }
}

void cache_release(cache *cache)
{
  FreeSlabsFromList(cache->free);
  cache->free = NULL;

  FreeSlabsFromList(cache->partlyFree);
  cache->partlyFree = NULL;

  FreeSlabsFromList(cache->booked);
  cache->booked = NULL;
}

bool CreateNewSlab(cache *cache)
{
  SLAB *tmp = (SLAB *)alloc_slab(cache->slab_order);
  if (tmp == NULL)
    return false;

  tmp->size = 0;
  if (cache->slab_objects > 1)
    tmp->isSlabMarked = false;
  else
    tmp->isSlabMarked = true;

  if (cache->slab_objects > 0)
  {
    tmp->memPart = (MEMPART *)(tmp + 1);
    tmp->memPart->next = NULL;
    tmp->memPart->prev = NULL;
  }
  else
  {
    tmp->memPart = NULL;
  }

  tmp->next = cache->free;
  tmp->prev = NULL;
  cache->free = tmp;

  return true;
}

void *AllocFromSlab(cache *cache, SLAB *slab)
{
  if (slab->memPart == NULL)
    return NULL;

  if (slab->memPart->next == NULL && slab->isSlabMarked == false)
  {
    slab->memPart->next = (MEMPART *)((size_t)(slab->memPart) + cache->object_size);
    slab->memPart->next->next = NULL;
    slab->memPart->next->prev = slab->memPart;
    if ((slab->size + 2) == cache->slab_objects)
      slab->isSlabMarked = true;
  }

  slab->size++;
  MEMPART *tmp = slab->memPart;
  slab->memPart = slab->memPart->next;
  if (slab->memPart != NULL)
    slab->memPart->prev = NULL;

  return (void *)tmp;
}

void MoveFirstElement(SLAB **from, SLAB **to)
{
  SLAB *tmp = *from;
  if (tmp->next != NULL)
    tmp->next->prev = NULL;
  *from = tmp->next;
  tmp->next = *to;
  if (*to != NULL)
    (*to)->prev = tmp;
  *to = tmp;
}

void *cache_alloc(cache *cache)
{
  if (cache->partlyFree != NULL)
  {
    void *ptr = AllocFromSlab(cache, cache->partlyFree);
    if (ptr == NULL)
      return NULL;

    if (cache->partlyFree->size == cache->slab_objects)
      MoveFirstElement(&cache->partlyFree, &cache->booked);

    return ptr;
  }
  else if (cache->free != NULL)
  {
    void *ptr = AllocFromSlab(cache, cache->free);
    if (ptr == NULL)
      return NULL;

    if (cache->free->size == cache->slab_objects)
      MoveFirstElement(&cache->free, &cache->booked);
    else
      MoveFirstElement(&cache->free, &cache->partlyFree);

    return ptr;
  }
  else
  {
    if (CreateNewSlab(cache) == false)
      return NULL;

    void *ptr = AllocFromSlab(cache, cache->free);
    if (ptr == NULL)
      return NULL;

    if (cache->free->size == cache->slab_objects)
      MoveFirstElement(&cache->free, &cache->booked);
    else
      MoveFirstElement(&cache->free, &cache->partlyFree);

    return ptr;
  }
}

void MoveSlab(SLAB *slab, SLAB **from, SLAB **to)
{
  if (slab->prev != NULL)
    slab->prev->next = slab->next;
  if (slab->next != NULL)
    slab->next->prev = slab->prev;

  if (*from == slab)
    *from = slab->next;

  slab->prev = NULL;
  slab->next = *to;
  if (*to != NULL)
    (*to)->prev = slab;
  *to = slab;
}

size_t GetNLowBits(size_t x, size_t n)
{
  return x & (~((1 << n) - 1));
}

void cache_free(cache *cache, void *ptr)
{
  SLAB *slab = (SLAB *)GetNLowBits((size_t)ptr, (size_t)(cache->slab_order + 12));
  MEMPART *memPart = (MEMPART *)ptr;
  memPart->prev = NULL;
  memPart->next = slab->memPart;
  if (slab->memPart != NULL)
    slab->memPart->prev = memPart;
  slab->memPart = memPart;

  if (slab->size == 1)
  {
    if (cache->slab_objects == 1)
      MoveSlab(slab, &cache->booked, &cache->free);
    else
      MoveSlab(slab, &cache->partlyFree, &cache->free);
  }
  else if (slab->size == cache->slab_objects)
  {
    if (cache->slab_objects == 1)
      MoveSlab(slab, &cache->booked, &cache->free);
    else
      MoveSlab(slab, &cache->booked, &cache->partlyFree);
  }

  slab->size--;
}

void cache_shrink(cache *cache)
{
  FreeSlabsFromList(cache->free);
  cache->free = NULL;
}