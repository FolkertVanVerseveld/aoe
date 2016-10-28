#ifndef AOE_DMAP_H
#define AOE_DMAP_H

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
	struct drsmap *dblk;
	int fd;
	struct drsmap *drs_data;
	char filename[DMAPBUFSZ];
	size_t length;
};

void dmap_init(struct dmap *map);
void dmap_free(struct dmap *map);
void *drs_get_item(unsigned item, int fd, size_t *count, off_t *offset);
void drs_free(void);

/*
read data file from specified path and save all data in drs_list.
provides additional error checking compared to original dissassembly
*/
int read_data_mapping(const char *filename, const char *directory, int no_stat);

#endif
