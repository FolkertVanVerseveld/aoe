/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef GENIE_SHIP_H
#define GENIE_SHIP_H

#include <stddef.h>
#include <sys/types.h>
#include "shape.h"
#include "fshape.h"

struct shp {
	void *file_slp_shp_data;
	size_t num4;
	off_t filesize;
	unsigned numC;
	struct shape *shp_shape;
	void *filedata2_off8;
	struct fshape *slp_shape;
	void *slp_off20;
};

struct shp *shp_init(struct shp *this, const char *fname, int res_id);
int shp_check_shape(struct shp *this, int vertices, const char *a3);
unsigned shp_check_shape_count(struct shp *this);

#endif
