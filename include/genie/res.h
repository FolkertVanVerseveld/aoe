/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * PE resource wrapper API
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 * Convient API for reading PE resources
 */

#ifndef RES_H
#define RES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#ifdef _WIN32
// since windows already provides dll resource extraction, don't bother using libpe

	#include <windows.h>

struct pe_lib {
	HMODULE module;
};

#else

	#include <libpe/pe.h>

struct pe_lib {
	pe_ctx_t ctx;
	NODE_PERES *res;
};

#endif

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
