/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * PE resource wrapper API
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 * Convenient API for reading PE resources
 */

#ifndef GENIE_RES_H
#define GENIE_RES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#ifndef RT_STRING
#define RT_UNKNOWN      0
#define RT_CURSOR       1
#define RT_BITMAP       2
#define RT_ICON         3
#define RT_MENU         4
#define RT_DIALOG       5
#define RT_STRING       6
#define RT_FONTDIR      7
#define RT_FONT         8
#define RT_ACCELERATOR  9
#define RT_RCDATA       10
#define RT_MESSAGETABLE 11
#define RT_GROUP_CURSOR 12
#define RT_GROUP_ICON   14
#define RT_VERSION      16
#define RT_DLGINCLUDE   17
#define RT_PLUGPLAY     19
#define RT_VXD          20
#define RT_ANICURSOR    21
#define RT_ANIICON      22
#endif

struct pe;

struct pe_lib {
	struct pe *pe;
};

/** Open and parse dll */
int pe_lib_open(struct pe_lib *lib, const char *name);

void pe_lib_close(struct pe_lib *lib);

/** Fetch text from resource table */
int load_string(struct pe_lib *lib, unsigned id, char *str, size_t size);

/** Fetch image from resource table */
int load_bitmap(struct pe_lib *lib, unsigned id, void **data, size_t *size);

#ifdef __cplusplus
}
#endif

#endif
