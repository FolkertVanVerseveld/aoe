#ifndef AOE_DBG_H
#define AOE_DBG_H

#ifdef DEBUG
#include <stdio.h>
#define dbgs(s) puts(s)
#define dbgf(f,...) printf(f,##__VA_ARGS__)
#else
#define dbgs(s) ((void)0)
#define dbgf(f,...) ((void)0)
#endif

void hexdump(const void *buf, size_t n);

#endif
