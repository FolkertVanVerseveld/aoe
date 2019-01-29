/* Copyright 2016-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/*
 * Data Resource Set file format
 *
 * Licensed under GNU Affero General Public License version 3
 * Copyright 2019 Folkert van Verseveld
 */

#ifndef GENIE_DRS_H
#define GENIE_DRS_H

#include "endian.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

struct drsmap {
	char copyright[40];
	char version[16];
	uint32_t nlist;
	uint32_t listend; // XXX go figure
};

/*
drs item types:

'bina' everything else
'wav ' audio files
'slp ' model files
'shp ' shape files
*/

#if GENIE_BYTE_ORDER_LITTLE
  #define DT_BINARY 0x62696e61
  #define DT_SHP    0x73687020
  #define DT_SLP    0x736c7020
  #define DT_WAVE   0x77617620
#else
  #define DT_BINARY 0x616e6962
  #define DT_SHP    0x20706873
  #define DT_SLP    0x20706c73
  #define DT_WAVE   0x20766177
#endif

struct drs_list {
	uint32_t type;
	uint32_t offset;
	uint32_t size;
};

struct drs_item {
	uint32_t id;
	uint32_t offset;
	uint32_t size;
};

#define DRS_NO_REF ((uint32_t)-1)
#define DRS_REF_NAMESZ 12

struct drs_ref {
	char name[DRS_REF_NAMESZ];
	uint32_t id;
};

struct drs_fref {
	char name[DRS_REF_NAMESZ];
	char name2[DRS_REF_NAMESZ];
	uint32_t id;
	uint32_t id2;
};

int drs_ref_read(const char *data, struct drs_ref *ref);
int drs_fref_read(const char *data, struct drs_fref *ref);

#endif
