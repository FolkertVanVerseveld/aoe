#ifndef AOE_DMAP_H
#define AOE_DMAP_H

/*
drs item types:

'bina'
'wav '
'slp '
'shp '
*/

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

struct drsmap {
	char copyright[40];
	char version[16];
	uint32_t nlist;
	uint32_t listend; // XXX go figure
};

#define DMAPBUFSZ 260
struct dmap {
	struct drsmap *dblk;
	int fd;
	struct drsmap *drs_data;
	char filename[DMAPBUFSZ];
	size_t length;
};

#define DT_BINARY 0x62696e61
#define DT_SHP    0x73687020
#define DT_SLP    0x736c7020
#define DT_WAVE   0x77617620

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
