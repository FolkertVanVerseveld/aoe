/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "shape.h"
#include "dbg.h"
#include <stdio.h>

int shape_read(const void *blk, struct shape *shape, size_t n)
{
	const char *data = blk;
	uint32_t *dw = (uint32_t*)data;
	if (n < sizeof(struct shape)) {
		fputs("bad shape\n", stderr);
		return 1;
	}
	shape->magic = dw[0];
	shape->pad[0] = dw[1];
	shape->pad[1] = dw[2];
	shape->pad[2] = dw[3];
	shape->data = data + 0x10;
	hexdump(blk, 0x20);
	return 0;
}
