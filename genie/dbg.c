/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include "dbg.h"

void hexdump(const void *buf, size_t n) {
	uint16_t i, j, p = 0;
	const unsigned char *data = buf;
	while (n) {
		printf("%04hX", p & ~0xf);
		for (j = p, i = 0; n && i < 0x10; ++i, --n)
			printf(" %02hhX", data[p++]);
		putchar(' ');
		for (i = j; i < p; ++i)
			putchar(isprint(data[i]) ? data[i] : '.');
		putchar('\n');
	}
}
