/* Copyright 2016-2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef DRS_H
#define DRS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define DRS_BACKGROUND_MAIN 50051

#define DRS_NO_REF ((uint32_t)-1)

// FIXME also support big-endian machines
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

#include <sys/types.h>

void drs_add(const char *name);

const void *drs_get_item(unsigned type, unsigned id, size_t *count);

void drs_init(void);
void drs_free(void);

#ifdef __cplusplus
}
#endif

#endif
