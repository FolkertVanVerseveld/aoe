#ifndef AOE_DMAP_H
#define AOE_DMAP_H

/*
drs item types:

'bina'
'wav '
'slp '
'shp '
*/

#include <stddef.h>
#include <sys/types.h>

struct drs_item {
	char hdr[40];
	char tribe[16];
	// REMAP typeof(count38) == unsigned
	unsigned count38;
	unsigned num3C;
	unsigned type40;
	unsigned num44;
	// REMAP typeof(count48) == unsigned
	unsigned count48;
};

#define DMAPBUFSZ 260
struct dmap {
	struct drs_item *dblk;
	int fd;
	struct drs_item *drs_data;
	struct dmap *next;
	char filename[DMAPBUFSZ];
	size_t length;
};

#define DT_BINARY 0x616e6962
#define DT_AUDIO 0x20766177
#define DT_SLP 0x20706c73
#define DT_SHP 0x20706873

void dmap_init(struct dmap *map);
void dmap_free(struct dmap *map);
void *drs_get_item(unsigned item, int fd, size_t *count, off_t *offset);
void drs_free(void);

/*
read data file from specified path and save all data in drs_list.
provides additional error checking compared to original dissassembly
*/
int read_data_mapping(const char *filename, const char *directory, int no_stat);

extern struct dmap *drs_list;

#endif
