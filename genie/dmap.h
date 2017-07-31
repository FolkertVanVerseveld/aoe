/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef GENIE_DMAP_H
#define GENIE_DMAP_H

#define DRS_UI "Interfac.drs"
#define DRS_BORDER "Border.drs"
#define DRS_MAP "Terrain.drs"
#define DRS_GFX "graphics.drs"
#define DRS_SFX "sounds.drs"

#define DRS_XDATA "data2/"
#define DRS_DATA "data/"

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "../genie/drs.h"

#define DMAPBUFSZ 260
struct dmap {
	char *data;
	int fd;
	size_t length;
	const char *filename;
	int nommap;
};

void dmap_set_list(struct dmap *list, unsigned count);
void dmap_list_free(void);
int dmap_list_init(void);

void *drs_get_item(unsigned item, int res_id, size_t *count, off_t *offset);
void drs_free(void);

/*
read data file from specified path and save all data in drs_list.
provides additional error checking compared to original dissassembly
*/
int read_data_mapping(const char *filename, const char *directory, int no_stat);

#endif