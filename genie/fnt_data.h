/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Prerendered font data
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 * Since libfreetype sucks we need to prerender all glyphs...
 */

#ifndef GENIE_FNT_DATA_H
#define GENIE_FNT_DATA_H

#include <stdint.h>

struct fnt_tm {
	int tm_height;
	int tm_ascent;
	int tm_descent;
	int tm_ilead;
	int tm_elead;
	int tm_avgchw;
	int tm_maxchw;
	int tm_weight;
	int tm_ohang;
	int tm_dasx;
	int tm_dasy;
	unsigned char tm_chrfst;
	unsigned char tm_chrend;
	unsigned char tm_chrdef;
	unsigned char tm_chrbrk;
	unsigned char tm_italic;
	unsigned char tm_uline;
	unsigned char tm_struck;
	unsigned char tm_paf;
	unsigned char tm_cs;
};

struct fnt_cs {
	int max_w, max_h;
	int data[2 * 256];
};

extern const struct fnt_tm fnt_tm_default, fnt_tm_default_bold, fnt_tm_button, fnt_tm_large;
extern const struct fnt_cs fnt_cs_default, fnt_cs_default_bold, fnt_cs_button, fnt_cs_large;

#endif
