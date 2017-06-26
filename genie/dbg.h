/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef GENIE_DBG_H
#define GENIE_DBG_H

#include <stddef.h>

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
