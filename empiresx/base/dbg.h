/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

#if __cplusplus
extern "C" {
#endif

#if DEBUG
#include <stdio.h>
#include <assert.h>

#define dbgf(f, ...) printf(f, ## __VA_ARGS__)
#define dbgs(s) puts(s)

#else

#define dbgf(f, ...) ((void)0)
#define dbgs(s) ((void)0)

#endif

#if __cplusplus
}
#endif
