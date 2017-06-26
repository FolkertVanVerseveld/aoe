/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "shp.h"
#include "dmap.h"
#include "dbg.h"
#include "todo.h"
#include "string.h"
#include "memmap.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static char palette_shp;
static unsigned ship_count;
static int shp_data554864;

struct shp *shp_init(struct shp *this, const char *fname, int res_id)
{
	int fd = -1;
	char filename[260];
	off_t filesize;
	struct fshape *slp_shape;
	struct shape *shp_data;
	this->file_slp_shp_data = NULL;
	this->numC = 0;
	this->shp_shape = NULL;
	this->filedata2_off8 = NULL;
	this->slp_shape = NULL;
	if (palette_shp || res_id < 0) {
		strcpy(filename, fname);
		strcpy(&filename[strlen(filename)], "SLP");
		++ship_count;
		fd = open(filename, O_RDONLY);
		if (fd != -1) {
			filesize = lseek(fd, 0, SEEK_END);
			this->slp_shape = malloc(filesize);
			assert(this->slp_shape);
			lseek(fd, 0, SEEK_SET);
			read(fd, this->slp_shape, filesize);
			this->num4 = 1;
			this->slp_off20 = (char*)this->slp_shape + 32;
			this->filesize = filesize;
		}
		// if .slp not found
		if (!this->slp_shape) {
			strcpy(&filename[strlen(filename)], "SHP");
			fd = open(filename, O_RDONLY);
			if (fd != -1) {
				filesize = lseek(fd, 0, SEEK_END);
				this->file_slp_shp_data = malloc(filesize);
				lseek(fd, 0, SEEK_SET);
				read(fd, this->file_slp_shp_data, filesize);
				this->shp_shape = this->file_slp_shp_data;
				this->filedata2_off8 = (char*)this->shp_shape + 8;
				this->num4 = 1;
				this->filesize = filesize;
			}
		}
	}
	if (!this->slp_shape && !this->file_slp_shp_data && res_id >= 0) {
		++ship_count;
		slp_shape = drs_get_item(DT_SLP, res_id, &this->num4, &this->filesize);
		this->file_slp_shp_data = slp_shape;
		if (slp_shape) {
			this->slp_shape = slp_shape;
			this->slp_off20 = (char*)slp_shape + 32;
			this->file_slp_shp_data = NULL;
			goto end;
		}
		shp_data = drs_get_item(DT_SHP, res_id, &this->num4, &this->filesize);
		this->file_slp_shp_data = shp_data;
		if (shp_data) {
			if (*((int*)shp_data) == shp_data554864) {
				this->shp_shape = shp_data;
				this->filedata2_off8 = (char*)shp_data + 8;
			}
		}
	}
end:
	if (fd != -1)
		close(fd);
	return this;
}

int shp_check_shape(struct shp *this, int vertices, const char *a3)
{
	char *str = NULL;
	if ((this->file_slp_shp_data == 0) != 0) {
		if (!this->slp_shape)
			goto fail;
	} else {
		struct shape *shape = this->shp_shape;
		if (!shape) {
			strmerge(&str, "ERROR: (", a3);
			strmerge(&str, str, ") Bad shape");
			goto err;
		}
		if (vertices >= 0 && vertices >= shape->vertices) {
			strmerge(&str, "ERROR: (", a3);
			strmerge(&str, str, ") shape number out of bounds");
			goto err;
		}
		if (shape->magic != SHAPE_V1_10) {
			strmerge(&str, "ERROR: (", a3);
			strmerge(&str, str, ") shape version wrong");
			goto err;
		}
		struct fshape *fshape = this->slp_shape;
		if (fshape && vertices >= 0 && vertices >= fshape->vertices) {
			strmerge(&str, "ERROR: (", a3);
			strmerge(&str, str, ") Fshape number out of bounds");
			goto err;
		}
	}
	return 0;
err:
	fprintf(stderr, "%s\n", str);
	delete(str);
fail:
	return 1;
}

unsigned shp_check_shape_count(struct shp *this)
{
	if (shp_check_shape(this, -1, "RGL_shape_count"))
		return 0;
	return this->slp_shape ? this->slp_shape->magic : this->shp_shape->magic;
}
