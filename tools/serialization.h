/* Copyright 2019 Folkert van Verseveld, zlibcomplete by Rudi Cilibrasi. License: MIT */

#ifndef AOE_TOOLS_INFLATE_H
#define AOE_TOOLS_INFLATE_H

/*
 * Copyright Folkert van Verseveld
 * Uses zlibcomplete which is written by Rudi Cilibrasi
 * 
 * License: MIT
 */

#ifdef __cplusplus
extern "C" {
#endif

int raw_inflate(void **dst, size_t *dst_size, const void *src, size_t size);

#ifdef __cplusplus
}
#endif

#endif
