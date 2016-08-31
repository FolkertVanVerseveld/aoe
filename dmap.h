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
	int count38;
	unsigned num3C;
	unsigned type40;
	unsigned num44;
	int count48;
};

#define DMAPBUFSZ 260
struct dmap {
	char *dblk;
	int fd;
	struct drs_item *drs_data;
	struct dmap *next;
	char filename[DMAPBUFSZ];
	size_t length;
};

#define DT_BINARY 'bina'
#define DT_AUDIO 'wav '
#define DT_SLP 'slp '
#define DT_SHP 'shp '

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
