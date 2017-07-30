/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Debug facilities
 *
 * Licensed under the GNU Affero General Public License version 3
 * Copyright 2017 Folkert van Verseveld
 */

#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
#include <stdio.h>
#define dbgs(s) puts(s)
#define dbgf(f,...) printf(f,##__VA_ARGS__)
#else
#define dbgs(s) ((void)0)
#define dbgf(f,...) ((void)0)
#endif

#define stub fprintf(stderr, "stub: %s\n", __func__);
#define FIXME(s) fprintf(stderr, "fixme: %s: %s\n", __func__, s)

#endif
