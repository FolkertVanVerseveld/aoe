#ifndef AOE_DMAP_H
#define AOE_DMAP_H

#include <stddef.h>

#define DMAPBUFSZ 260
struct dmap {
	char *dblk;
	int fd;
	char *drs_data;
	struct dmap *next;
	char filename[DMAPBUFSZ];
	size_t length;
};

void dmap_init(struct dmap *map);
void dmap_free(struct dmap *map);
void drs_free(void);

/*
read data file from specified path and save all data in drs_list.
provides additional error checking compared to original dissassembly
*/
int read_data_mapping(const char *filename, const char *directory, int no_stat);

extern struct dmap *drs_list;

#endif
