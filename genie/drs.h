#ifndef GENIE_DRS_H
#define GENIE_DRS_H

/*
drs item types:

'bina'
'wav '
'slp '
'shp '
*/

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

struct drsmap {
	char copyright[40];
	char version[16];
	uint32_t nlist;
	uint32_t listend; // XXX go figure
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

#endif
