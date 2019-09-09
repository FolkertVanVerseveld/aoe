/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef GENIE_MEMORY_H
#define GENIE_MEMORY_H

#ifndef __cplusplus

#include <stddef.h>

void *mem_alloc(size_t size);
void *mem_realloc(void *ptr, size_t size);
void mem_free(void *ptr);

#endif

#endif
