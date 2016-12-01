#ifndef GENIE_SHAPE_H
#define GENIE_SHAPE_H

#include <stdint.h>

struct shape {
	uint32_t magic;
	uint32_t pad[3];
	const char *data;
};

int shape_read(const void *data, struct shape *shape, size_t n);

/*
0000000: 312e 3130 0100 0000 1000 0000 0000 0000  1.10............
0000010: 0d00 0d00 0000 0000 0200 0000 0200 0000  ................
0000010: 5802 2003 0000 0000 0000 0000 0000 0000  X. .............
0000010: e001 8002 0000 0000 0000 0000 0000 0000  ................
*/

#endif
