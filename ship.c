#include "ship.h"
#include "dbg.h"
#include "todo.h"

struct ship_20 *shp_init(struct ship_20 *this, const char *fname, int res_id)
{
	stub
	this->file_slp_shp_data = NULL;
	this->numC = 0;
	this->filedata2 = NULL;
	this->filedata2_off8 = NULL;
	this->slp_data = NULL;
	return this;
}
