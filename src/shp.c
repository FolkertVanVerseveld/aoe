#include "shp.h"
#include "dmap.h"
#include "dbg.h"
#include "todo.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static char ship_7C0464;
static unsigned ship_count;
static int shp_data554864;

struct ship_20 *shp_init(struct ship_20 *this, const char *fname, int res_id)
{
	stub
	int fd = -1;
	char filename[260];
	off_t filesize;
	void *slp_data, *shp_data;
	this->file_slp_shp_data = NULL;
	this->numC = 0;
	this->filedata2 = NULL;
	this->filedata2_off8 = NULL;
	this->slp_data = NULL;
	if (ship_7C0464 || res_id < 0) {
		strcpy(filename, fname);
		strcpy(&filename[strlen(filename)], "SLP");
		++ship_count;
		fd = open(filename, O_RDONLY);
		if (fd != -1) {
			filesize = lseek(fd, 0, SEEK_END);
			this->slp_data = malloc(filesize);
			assert(this->slp_data);
			lseek(fd, 0, SEEK_SET);
			read(fd, this->slp_data, filesize);
			this->num4 = 1;
			this->slp_data_off20 = (char*)this->slp_data + 32;
			this->filesize = filesize;
		}
		// if .slp not found
		if (!this->slp_data) {
			strcpy(&filename[strlen(filename)], "SHP");
			fd = open(filename, O_RDONLY);
			if (fd != -1) {
				filesize = lseek(fd, 0, SEEK_END);
				this->file_slp_shp_data = malloc(filesize);
				lseek(fd, 0, SEEK_SET);
				read(fd, this->file_slp_shp_data, filesize);
				this->filedata2 = this->file_slp_shp_data;
				this->filedata2_off8 = (char*)this->filedata2 + 8;
				this->num4 = 1;
				this->filesize = filesize;
			}
		}
	}
	if (!this->slp_data && !this->file_slp_shp_data && res_id >= 0) {
		++ship_count;
		slp_data = drs_get_item(DT_SLP, res_id, &this->num4, &this->filesize);
		this->file_slp_shp_data = slp_data;
		if (slp_data) {
			this->slp_data = slp_data;
			this->slp_data_off20 = (char*)slp_data + 32;
			this->file_slp_shp_data = NULL;
			goto end;
		}
		shp_data = drs_get_item(DT_SHP, res_id, &this->num4, &this->filesize);
		this->file_slp_shp_data = shp_data;
		if (shp_data) {
			if (*((int*)shp_data) == shp_data554864) {
				this->filedata2 = shp_data;
				this->filedata2_off8 = (char*)shp_data + 8;
			}
		}
	}
end:
	if (fd != -1)
		close(fd);
	return this;
}
