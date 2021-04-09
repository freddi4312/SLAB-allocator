#pragma once

void *alloc_slab(int order);
void free_slab(void *slab);