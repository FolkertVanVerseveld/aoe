#ifndef AOE_SHIP_H
#define AOE_SHIP_H

#include <stddef.h>
#include <sys/types.h>

struct shp {
	void *file_slp_shp_data;
	size_t num4;
	off_t filesize;
	unsigned numC;
	void *filedata2, *filedata2_off8;
	void *slp_data, *slp_data_off20;
};

struct shp *shp_init(struct shp *this, const char *fname, int res_id);

#endif
