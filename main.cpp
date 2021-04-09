#include <cstdlib>
#include <algorithm>
#include "Slab.h"


int main(void)
{
  cache cache;
  void *ptr1, *ptr2, *ptr3, *ptr4, *ptr5;

  cache_setup(&cache, 2000000);
  ptr1 = cache_alloc(&cache);
  ptr2 = cache_alloc(&cache);
  ptr3 = cache_alloc(&cache);
  cache_free(&cache, ptr1);
  ptr1 = cache_alloc(&cache);
  ptr4 = cache_alloc(&cache);
  ptr5 = cache_alloc(&cache);

  cache_free(&cache, ptr1);
  cache_free(&cache, ptr2);
  cache_free(&cache, ptr3);
  cache_release(&cache);

  return 1;
}