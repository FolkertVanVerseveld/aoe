/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <genie/memory.h>

#include <genie/def.h>

#include <stdlib.h>

void *mem_alloc(size_t size)
{
	void *ptr;

	if (!(ptr = malloc(size)))
		panic("Out of memory");

	return ptr;
}

void *mem_realloc(void *ptr, size_t size)
{
	void *blk;

	if (!(blk = realloc(ptr, size)))
		panic("Out of memory");

	return blk;
}

void mem_free(void *ptr)
{
	free(ptr);
}
