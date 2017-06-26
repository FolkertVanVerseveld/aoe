/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef GENIE_RSRC_H
#define GENIE_RSRC_H

#define HEAPSZ 65536

#include <stdint.h>

struct rsrcstr {
	uint16_t length;
	char *str;
};

struct rstrptr {
	unsigned id;
	struct rsrcstr str;
};

extern struct strheap {
	struct rstrptr a[HEAPSZ];
	unsigned n;
	unsigned str_id;
} strtbl;

#endif
