#ifndef AOE_MEMMAP_H
#define AOE_MEMMAP_H

#include <stddef.h>

void meminit(void);

void *new(size_t n);
void nuke(void *ptr);
#define delete(p) do{nuke(p);p=NULL;}while(0)

#endif
